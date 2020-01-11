import errno
import os
import re
import requests
import sys


def download_pdb(name, guid):
    filename = 'symbols/{name}/{guid}/{name}'.format(name=name, guid=guid)
    if os.path.isfile(filename):
        return

    url = 'http://msdl.microsoft.com/download/' + filename
    user_agent = 'Microsoft-Symbol-Server/6.11.0001.404'
    headers = ({'User-Agent': user_agent})
    r = requests.get(url, headers=headers, allow_redirects=True)
    print('{code} {url}'.format(code=r.status_code, url=url))
    if r.status_code != 200:
        return -1

    if not os.path.exists(os.path.dirname(filename)):
        try:
            os.makedirs(os.path.dirname(filename))
        except OSError as exc:
            if exc.errno != errno.EEXIST:
                raise
    with open(filename, 'wb') as fh:
        fh.write(r.content)


def read_file(filename):
    with open(filename, "rb") as fh:
        return fh.read().decode()


def download_pdbs(filename):
    r_pdb = r".*unable to open pdb.+[\\\/](.*\.pdb)[\\\/]([A-F0-9]+).*"
    data = read_file(filename)
    for pdb in re.finditer(r_pdb, data):
        download_pdb(pdb.group(1), pdb.group(2))
    return 0


if __name__ == "__main__":
    ret = download_pdbs(sys.argv[1])
    sys.exit(ret)
