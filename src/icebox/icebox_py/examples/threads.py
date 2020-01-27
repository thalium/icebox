import icebox

vm = icebox.attach("win10")

# list current threads
proc = vm.processes.current()
for thread in proc.threads():
    print("%s: %d" % (proc.name(), thread.tid()))

thread = vm.threads.current()  # get current thread
proc_bis = thread.process()
assert(proc == proc_bis)


def on_create(thread):
    print("+ %s: %d" % (thread.process().name(), thread.tid()))


def on_delete(p):
    print("- %s: %d" % (thread.process().name(), thread.tid()))


# put breakpoints on thread creation & deletion
with vm.threads.break_on_create(on_create):
    with vm.threads.break_on_delete(on_delete):
        for i in range(0, 4):
            vm.exec()
