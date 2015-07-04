#!/usr/bin/env python
from __future__ import absolute_import, print_function, with_statement

import sys, os, re, optparse
import xml.etree.ElementTree as ET

def load_junit_xmlfile(xml_file):
    tree = ET.parse(xml_file)
    root = tree.getroot()
    if cmp(root.tag, "testsuite") != 0:
        return 0

    testsuites = root.findall('.')

    for testsuite in testsuites:
        name = testsuite.get('name')
        test_num = testsuite.get("tests")
        fail_num = testsuite.get("failures")
        error_num = testsuite.get("errors")
        skipped_num = testsuite.get("skipped")
        cost_time = testsuite.get("time")

        if int(fail_num) > 0 or int(error_num) > 0:
            print("Python_Check Failed: name {0} tests {1} failures {2} errors {3} skipped {4} time {5}".format(name, test_num, fail_num, error_num, skipped_num, cost_time))
            return 1
    return 0

def main():
    usage = "usage: %prog -d hbase_dir"
    parser = optparse.OptionParser(usage)
    parser.add_option("-d", "--dir", action="store", type="string", default="./", dest="project_dir")
    (options, args) = parser.parse_args()

    for dir in os.listdir(options.project_dir):
        xml_report_path = options.project_dir + "/" + dir + "/target/surefire-reports/"
        if not os.path.isdir(xml_report_path):
            continue
        for filename in os.listdir(xml_report_path):
            if re.match(".*.xml$", filename):
                if (load_junit_xmlfile(xml_report_path + filename) != 0):
                    print("load_junit_xmlfile: %s" % filename)
                    sys.exit(1)

if __name__ == "__main__":
    main()

