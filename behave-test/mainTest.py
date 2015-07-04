from __future__ import absolute_import, print_function, with_statement

import os, sys, argparse, time, re
import json, optparse, yaml
import pprint

from mod_utils import command_shell
from mod_utils import sort_and_diff
from mod_utils import textutil
from mod_utils import junit

class GenerateTest(object):
    def __init__(self):
        pass

    def load_config(self):
        with open("conf.yml") as f:
            self.conf = yaml.load(f)
            self.hostname = "localhost"
            self.database = "default"
            # pprint.pprint(self.conf)

    def run_case(self, case_name, cmd_result = None):
        """

        :rtype : TestCase
        """
        if not cmd_result:
            case = junit.TestCase(case_name)
            return case
        if cmd_result.returncode == 0:
            status = "success"
        else:
            status = "failed"
        #cmd_result.show(False)
        case = junit.TestCase(case_name, status, cmd_result.time_delta, cmd_result.stdout, cmd_result.stderr)
        return case

    def generate_reports(self, suite, cases):
        report = junit.JUnitReporter(suite)
        for testcase in cases:
            report.add_caseresult(testcase)
        report.generate_reporter(suite)

class TranswarpTest(GenerateTest):
    def __init__(self):
        super(TranswarpTest, self).__init__()
        self.phasename = [
            "00_data_init",
            "01_hyperbase_ddl",
            "02_inceptor_ddl",
            "03_inceptor_insert",
            "04_query",
            "05_answer"
        ]
        self.transwarp_suites = list()
        self.jdbc_suites = list()
        self.workdir = os.getcwd()
        self.cur_suite = ""

    def _parse_conf(self):
        self.transwarp_suites = list()
        self.jdbc_suites = list()
        for suites_conf in self.conf:
            if not suites_conf.has_key("transwarp_suites"):
                continue
            if suites_conf.has_key("hostname"):
                self.hostname = suites_conf['hostname']
            if suites_conf.has_key("database"):
                self.database = suites_conf['database']
            if suites_conf.has_key("transwarp_suites"):
                self.transwarp_suites = suites_conf["transwarp_suites"]
            if suites_conf.has_key("rootdir"):
                self.workdir = os.path.join(suites_conf["rootdir"])
                print("WORKDIR %s" % self.workdir)
            if suites_conf.has_key("jdbc_suites"):
                self.jdbc_suites = suites_conf["jdbc_suites"]

    def _data_init(self, params):
        cases = list()
        local_dir = None
        hdfs_dir = None
        if not params:
            return

        for param in params:
            if param.has_key('local_dir'):
                local_dir = param['local_dir']
            if param.has_key('hdfs_dir'):
                hdfs_dir = param['hdfs_dir']

        command_workflow = list()
        command = dict(name = "remove_hdfs", cmd = "hdfs dfs -rmr %s" % hdfs_dir)
        command_workflow.append(command)
        command = dict(name = 'mkdir_cmd', cmd = "hdfs dfs -mkdir -p %s" % hdfs_dir)
        command_workflow.append(command)
        command = dict(name = 'chmod_cmd', cmd = "hdfs dfs -chmod -R 777 %s" % hdfs_dir)
        command_workflow.append(command)
        command = dict(name = 'put_cmd', cmd = "hdfs dfs -put %s %s" % (local_dir, hdfs_dir))
        command_workflow.append(command)
        command = dict(name = 'chmod_cmd2', cmd = "hdfs dfs -chmod -R 777 %s" % hdfs_dir)
        command_workflow.append(command)

        path = os.path.join(self.workdir, self.cur_suite, self.phasename[0])
        start_time = time.time()
        for command in command_workflow:
            print("DataInit: " + command['cmd'])
            cmdresult = command_shell.Command.run(command['cmd'].split(), cwd=path)
            case = self.run_case(command['name'], cmdresult)
            cases.append(case)
        end_time = time.time()
        test_suites = junit.TestSuites(self.cur_suite, 'data_init', end_time - start_time)
        self.generate_reports(test_suites, cases)

    def _hyperbase_ddl(self, params):
        cases = list()
        prefix_command = "hbase shell"
        sql_files = ["initialize_hbase.sql",]

        if params:
            print(params)

        start_time = time.time()
        for sql_file in sql_files:
            path = os.path.join(self.workdir, self.cur_suite, self.phasename[1])
            if not os.path.isfile(os.path.join(path, sql_file)):
                case = self.run_case(sql_file.split('.')[0])
                cases.append(case)
                continue
            #ToDo: Run command interative
            command = "bash %s %s" % (os.path.join(os.getcwd(), self.workdir, "runner/hbase/initialize_hyperbase.sh"), sql_file)
            print("HBase DDL: " + command)
            cmdresult = command_shell.Command.run(command.split(), cwd=path)
            case = self.run_case(sql_file.split('.')[0], cmdresult)
            cases.append(case)
        end_time = time.time()
        test_suites = junit.TestSuites(self.cur_suite, 'hyperbase_ddl', end_time - start_time)
        self.generate_reports(test_suites, cases)

    def _inceptor_ddl(self, params):
        cases = list()
        prefix_command = "transwarp -t -h %s -f" % self.hostname
        sql_files = ["initialize_inceptor.sql",]

        if params:
            for param in params:
                if param.has_key('sqlfile'):
                    sql_files = param['sqlfile']

        start_time = time.time()
        for sql_file in sql_files:
            path = os.path.join(self.workdir, self.cur_suite, self.phasename[2])
            if not os.path.isfile(os.path.join(path, sql_file)):
                case = self.run_case(sql_file.split('.')[0])
                cases.append(case)
                continue
            command = "%s %s" % (prefix_command, sql_file)
            print("InceptorDDL: " + command)
            cmdresult = command_shell.Command.run(command.split(), cwd=path)
            case = self.run_case(sql_file.split('.')[0], cmdresult)
            cases.append(case)
        end_time = time.time()
        test_suites = junit.TestSuites(self.cur_suite, 'inceptor_ddl', end_time - start_time)
        self.generate_reports(test_suites, cases)

    def _inceptor_insert(self, params):
        cases = list()
        prefix_command = "transwarp -t -h %s -f" % self.hostname
        sql_files = ["insert.sql",]

        if params:
            print(params)

        start_time = time.time()
        for sql_file in sql_files:
            path = os.path.join(self.workdir, self.cur_suite, self.phasename[3])
            if not os.path.isfile(os.path.join(path, sql_file)):
                case = self.run_case(sql_file.split('.')[0])
                cases.append(case)
                continue
            command = "%s %s" % (prefix_command, sql_file)
            print("InceptorInsert: " + command)
            cmdresult = command_shell.Command.run(command.split(), cwd=path)
            case = self.run_case(sql_file.split('.')[0], cmdresult)
            cases.append(case)
        end_time = time.time()
        test_suites = junit.TestSuites(self.cur_suite, 'inceptor_insert', end_time - start_time)
        self.generate_reports(test_suites, cases)

    def load_config(self):
        super(TranswarpTest, self).load_config()
        self._parse_conf()

    def prepare_data(self, configrations):
        # for config in configrations:
        if not configrations:
            return

        for config in configrations:
            print(config)
            continue
            params = None
            if config.has_key(self.phasename[0]):
                params = config[self.phasename[0]]
            self._data_init(params)
            params = None
            if config.has_key(self.phasename[1]):
                params = config[self.phasename[1]]
            self._hyperbase_ddl(params)
            params = None
            if config.has_key(self.phasename[2]):
                params = config[self.phasename[2]]
            self._inceptor_ddl(params)
            params = None
            if config.has_key(self.phasename[3]):
                params = config[self.phasename[3]]
            self._inceptor_insert(params)
            # print(self.cur_suite, params)

    def do_all_tests(self):
        for transwarp_case in self.transwarp_suites:
            self._do_transwarp_test(transwarp_case)
        for jdbc_case in self.jdbc_suites:
            self._do_jdbc_test(jdbc_case)

    def _do_transwarp_sqls(self, prefix_command):
        # prefix_command = "transwarp -t -h %s -database %s -f" % (self.hostname, self.database)
        cases = list()
        path = os.path.join(self.workdir, self.cur_suite, self.phasename[4])

        start_time = time.time()
        print(os.getcwd()+'/' + path)
        for root, dirs, files in os.walk(path):
            for file_name in files:
                if not re.match(".*\.sql$", file_name):
                    continue
                command = "%s %s" % (prefix_command, file_name)
                print(command)
                cmdresult = command_shell.Command.run(command.split(), cwd=root)
                case = self.run_case(file_name.split('.')[0], cmdresult)
                with open(os.path.join(root, file_name), 'r') as f:
                    file_content = f.read()
                case.set_filecontent(file_content)
                if cmdresult.returncode != 0:
                    case.set_message(file_content)
                    cases.append(case)
                    continue

                # differ with answer
                answer_file = os.path.join(path, "../05_answer/%s.out" % file_name)
                diff_content = sort_and_diff.SortDiff.sort_differ_buffers(cmdresult.stdout, answer_file)
                if len(diff_content) > 512:
                    diff_content = diff_content[:512] + "...\nTruncated"

                case = self.run_case(file_name.split('.')[0], cmdresult)
                case.set_type("Different With Right Result!")
                case.set_message("Different With Right Result!")
                case.set_failmsg(diff_content)
                cases.append(case)
        end_time = time.time()
        test_suites = junit.TestSuites(self.cur_suite, 'exec_and_compare_results', end_time - start_time)
        self.generate_reports(test_suites, cases)

    def _do_transwarp_test(self, transwarp_case):
        prefix_command = "transwarp -t -h %s -database %s -f" % (self.hostname, self.database)
        for suite_name, configurations in transwarp_case.iteritems():
            self.cur_suite = suite_name
            self.prepare_data(configurations)
            self._do_transwarp_sqls(prefix_command)

    def _do_jdbc_test(self, jdbc_case):
        prefix_command = "bash runner/jdbc-runner-1.0.0/runJdbc.sh"
        for suite_name, configurations in jdbc_case.iteritems():
            self.cur_suite = suite_name
            self.prepare_data(configurations)
            self._do_transwarp_sqls(prefix_command)

class InceptorUnittest(object):
    def __init__(self):
        pass

if __name__ == "__main__":
    usage = "usage: %prog -d workdir"
    parser = optparse.OptionParser(usage)
    parser.add_option("-d", "--dir", action="store", type="string", default="./", dest="project_dir")
    (options, args) = parser.parse_args()

    hyper = TranswarpTest()
    hyper.load_config()
    os.chdir(options.project_dir)
    hyper.do_all_tests()
