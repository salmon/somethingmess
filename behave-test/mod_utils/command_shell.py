from __future__ import absolute_import, print_function, with_statement
import six
import os, sys, time, re
import subprocess

class CommandResult(object):

    def __init__(self, **kwargs):
        self.command = kwargs.pop("command", None)
        self.returncode = kwargs.pop("returncode", 0)
        self.stdout = kwargs.pop("stdout", "")
        self.stderr = kwargs.pop("stderr", "")
        self.time_delta = 0
        self.__output = None
        if kwargs:
            names = ", ".join(kwargs.keys())
            raise ValueError("Unexpected: %s" % names)

    @property
    def output(self):
        if self.__output is None:
            output = self.stdout
            if self.stderr:
                output += "\n"
                output += self.stderr
            self.__output = output
        return self.__output

    @property
    def failed(self):
        return self.returncode != 0

    def clear(self):
        self.command = None
        self.returncode = 0
        self.stdout = ""
        self.stderr = ""
        self.__output = 0

    def show(self, only_err = True):
        print("# shell.command={0}".format(self.command))
        print("# shell.command.output: returncode {0}\n".format(self.returncode))
        if only_err:
            if len(self.stderr) != 0:
                for line in self.stderr.split("\n"):
                    print("> stderr: {0}".format(line))
        elif len(self.stdout) != 0:
            for line in self.stdout.split("\n"):
                print("> {0}".format(line.encode('utf-8')))



class Command(object):

    @classmethod
    def run(cls, command, cwd=".", **kwargs):
        command_result = CommandResult()
        command_result.command = command

        start_time = time.time()
        try:
            process = subprocess.Popen(command,
                                       stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE,
                                       universal_newlines=True,
                                       cwd=cwd, **kwargs
                                       )
            out, err = process.communicate()
            if six.PY2:
                default_encoding = "UTF-8"
                out = six.text_type(out, process.stdout.encoding or default_encoding)
                err = six.text_type(err, process.stderr.encoding or default_encoding)
            process.poll()
            command_result.stdout = out
            command_result.stderr = err
            command_result.returncode = process.returncode

        except OSError as e:
            command_result.stderr = u"OSError: %s" % e
            command_result.returncode = e.errno
        end_time = time.time()

        time_delta = end_time - start_time
        command_result.time_delta = time_delta
        return command_result

    @classmethod
    def run_interactive(cls, command, file, prefix, cwd=".", **kwargs):
        content = ""
        command_result = CommandResult()
        command_result.command = command

        start_time = time.time()
        with open(file, "r") as f:
            content = f.readlines()
        try:
            process = subprocess.Popen(command,
                                       stdin=subprocess.PIPE,
                                       stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE,
                                       universal_newlines=True,
                                       cwd=cwd, **kwargs
                                       )
            out, err = process.communicate()
            if re.match(prefix, out):
                for line in content:
                    process.stdin.write(line)
            if six.PY2:
                default_encoding = "UTF-8"
                out = six.text_type(out, process.stdout.encoding or default_encoding)
                err = six.text_type(err, process.stderr.encoding or default_encoding)
            process.poll()
            command_result.stdout = out
            command_result.stderr = err
            command_result.returncode = process.returncode

        except OSError as e:
            command_result.stderr = u"OSError: %s" % e
            command_result.returncode = e.errno
        end_time = time.time()

        time_delta = end_time - start_time
        command_result.time_delta = time_delta
        return command_result
def run(commmand, cwd=".", **kwargs):
    return Command.run(commmand, cwd=cwd, **kwargs)

# Test
if __name__ == "__main__":
    command = " ".join(sys.argv[1:])
    output = Command.run(sys.argv[1:])
    print("command: {0}\n{1}\n".format(command, output))
    output.show()
