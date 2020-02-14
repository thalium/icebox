import argparse
import errno
import os
import re
import requests
import subprocess
import sys
import tqdm
import concurrent.futures as fut


def sizeof_fmt(num, suffix='B'):
    for unit in ['', 'Ki', 'Mi', 'Gi', 'Ti', 'Pi', 'Ei', 'Zi']:
        if abs(num) < 1024.0:
            return "%3.1f%s%s" % (num, unit, suffix)
        num /= 1024.0
    return "%.1f%s%s" % (num, 'Yi', suffix)


def download_file(url, guid, name, headers, dst, args):
    r = requests.get(url, headers=headers, allow_redirects=True, stream=True)
    if r.status_code != 200:
        return r.status_code

    total_size = int(r.headers.get('content-length', 0))
    if total_size > args.max_size:
        raise BaseException("skip:  %s %s: %s" %
                            (guid, name, sizeof_fmt(total_size)))

    if not os.path.exists(os.path.dirname(dst)):
        try:
            os.makedirs(os.path.dirname(dst))
        except OSError as exc:
            if exc.errno != errno.EEXIST:
                raise

    data = b""
    with tqdm.tqdm(desc="%s %s" % (guid, name), total=total_size,
                   ascii=True, unit='B', unit_scale=True) as pbar:
        for chunk in r.iter_content(chunk_size=4*1024):
            if chunk:
                data += chunk
                pbar.update(len(chunk))

    with open(dst, "wb") as fh:
        fh.write(data)

    return 0


def download_pdb(dst, name, guid, args):
    filename = os.path.join(dst, name, guid, name)
    if os.path.isfile(filename):
        return

    path = 'symbols/{name}/{guid}/{name}'.format(name=name, guid=guid)
    url = 'http://msdl.microsoft.com/download/' + path
    user_agent = 'Microsoft-Symbol-Server/6.11.0001.404'
    headers = ({'User-Agent': user_agent})
    err = download_file(url, guid, name, headers, filename, args)
    if err:
        raise BaseException("error: %s %s err:%d" % (guid, name, err))


def try_download_pdb(dst, name, guid, args):
    try:
        download_pdb(dst, name, guid, args)
        return 0
    except BaseException as exc:
        print(exc)
        return -1


def read_lines(filename):
    if filename == "-":
        for line in sys.stdin.readlines():
            yield line.rstrip()
        return

    with open(filename, "rb") as fh:
        for line in fh.readlines():
            yield line.decode().rstrip()


r_pdb = r".*unable to open pdb.+[\\\/](.*\.pdb)[\\\/]([A-F0-9]+).*"


def read_pdbs(text):
    for pdb in re.finditer(r_pdb, text):
        yield pdb.group(1), pdb.group(2)


def list_pdbs(filename):
    for line in read_lines(filename):
        for name, guid in read_pdbs(line):
            yield name, guid


def read_symbol_path():
    dst = os.environ["_NT_SYMBOL_PATH"]
    if not len(dst):
        raise BaseException("missing _NT_SYMBOL_PATH environment variable")
        return None

    dst = os.path.abspath(dst)
    print("_NT_SYMBOL_PATH=%s" % dst)
    return dst


def download_pdb_from_guid(args):
    dst = read_symbol_path()
    return try_download_pdb(dst, args.name, args.guid, args)


def download_pdbs_from_log(args):
    dst = read_symbol_path()
    with fut.ThreadPoolExecutor(max_workers=os.cpu_count()) as executor:
        futures = []
        for name, guid in list_pdbs(args.file):
            fret = executor.submit(download_pdb, dst, name, guid, args)
            futures.append(fret)

        count = 0
        for fret in fut.as_completed(futures):
            try:
                fret.result()
                count += 1
            except BaseException as exc:
                print(exc)
        return count


r_manifest_pdb = r"(.+\.pdb),([A-F0-9]+),(.+)"


def read_manifest_pdbs(text):
    for pdb in re.finditer(r_manifest_pdb, text):
        yield pdb.group(1), pdb.group(2)


def list_manifest_pdbs(filename):
    for line in read_lines(filename):
        for name, guid in read_manifest_pdbs(line):
            yield name, guid


def download_pdbs_from_manifest(args):
    dst = read_symbol_path()
    with fut.ThreadPoolExecutor(max_workers=os.cpu_count()) as executor:
        futures = []
        for name, guid in list_manifest_pdbs(args.file):
            fret = executor.submit(download_pdb, dst, name, guid, args)
            futures.append(fret)

        count = 0
        for fret in fut.as_completed(futures):
            try:
                fret.result()
                count += 1
            except BaseException as exc:
                print(exc)
        return count


load_all_symbols = """
import icebox

print("loading drivers")
vm = icebox.attach("{vm_name}")
vm.symbols.load_drivers()

print("loading explorer.exe symbols...")
p = vm.processes.wait("explorer.exe", icebox.flags_any)
p.join_user()
p.symbols.load_modules()

print("loading taskmgr.exe wow64 symbols...")
p = vm.processes.find_name("Taskmgr.exe", icebox.flags_x86)
p.join_user()
p.symbols.load_modules()
"""


def check_script(args):
    txt = load_all_symbols.format(vm_name=args.vm_name)
    return subprocess.check_call([sys.executable, "-c", txt])


def binexec(*args):
    try:
        ret = subprocess.check_output([*args], stderr=subprocess.STDOUT)
        return ret.decode()
    except subprocess.CalledProcessError as err:
        return err.output.decode()


def exec_once(dst, script, args):
    output = binexec(sys.executable, "-c", script)
    with fut.ThreadPoolExecutor(max_workers=os.cpu_count()) as executor:
        futures = []
        for name, guid in read_pdbs(output):
            fret = executor.submit(download_pdb, dst, name, guid, args)
            futures.append(fret)

        count = 0
        for fret in fut.as_completed(futures):
            try:
                fret.result()
                count += 1
            except BaseException as exc:
                print(exc)
        return count


def download_pdbs_from_vm(args):
    dst = read_symbol_path()
    txt = load_all_symbols.format(vm_name=args.vm_name)
    # run script until we cannot read anymore pdbs
    while True:
        ret = exec_once(dst, txt, args)
        if ret == 0:
            return 0


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(help='sub-commands')
    max_size = 30*1024*1024
    max_size_help = "max pdb size to download in bytes"

    # pdb
    pdb_cmd = subparsers.add_parser("pdb", help="download pdb from guid")
    pdb_cmd.add_argument("guid", type=str, help='guid name')
    pdb_cmd.add_argument("name", type=str, help='module name')
    pdb_cmd.add_argument("--max-size", type=int,
                         default=max_size, help=max_size_help)
    pdb_cmd.set_defaults(func=download_pdb_from_guid)

    # pdbs
    from_pdbs = subparsers.add_parser("pdbs", help="download pdbs from log")
    from_pdbs.add_argument("file", type=str, help="filename or - for stdin")
    from_pdbs.add_argument("--max-size", type=int,
                           default=max_size, help=max_size_help)
    from_pdbs.set_defaults(func=download_pdbs_from_log)

    # download
    download = subparsers.add_parser("download", help="download pdbs from vm")
    download.add_argument("vm_name", type=str, help="vm name")
    download.add_argument("--max-size", type=int, default=max_size,
                          help=max_size_help)
    download.set_defaults(func=download_pdbs_from_vm)

    # manifest
    manifest = subparsers.add_parser("manifest", help="download pdbs from manifest")
    manifest.add_argument("file", type=str, help="filename or - for stdin")
    manifest.add_argument("--max-size", type=int,
                           default=max_size, help=max_size_help)
    manifest.set_defaults(func=download_pdbs_from_manifest)

    # check
    check = subparsers.add_parser("check", help="check load script")
    check.add_argument("vm_name", type=str, help="vm name")
    check.set_defaults(func=check_script)

    # run input command
    args = parser.parse_args()
    ret = args.func(args)
    sys.exit(ret)
