# -*- coding: utf-8 -*-

from __future__ import absolute_import
import os.path
import codecs
import sys
from xml.etree import ElementTree
from .textutil import text as _text
import six


def CDATA(text=None):
    # -- issue #70: remove_ansi_escapes(text)
    element = ElementTree.Element('![CDATA[')
    element.text = text
    return element


class ElementTreeWithCDATA(ElementTree.ElementTree):
    def _write(self, file, node, encoding, namespaces):
        """This method is for ElementTree <= 1.2.6"""

        if node.tag == '![CDATA[':
            text = node.text.encode(encoding)
            file.write("\n<![CDATA[%s]]>\n" % text)
        else:
            ElementTree.ElementTree._write(self, file, node, encoding,
                                           namespaces)

if hasattr(ElementTree, '_serialize'):

    def _serialize_xml2(write, elem, encoding, qnames, namespaces,
                        orig=ElementTree._serialize_xml):
        if elem.tag == '![CDATA[':
            write("\n<%s%s]]>\n" % (elem.tag, elem.text.encode(encoding, "xmlcharrefreplace")))
            return
        return orig(write, elem, encoding, qnames, namespaces)

    def _serialize_xml3(write, elem, qnames, namespaces,
                        short_empty_elements=None,
                        orig=ElementTree._serialize_xml):
        if elem.tag == '![CDATA[':
            write("\n<{tag}{text}]]>\n".format(
                tag=elem.tag, text=elem.text))
            return
        if short_empty_elements:
            # python >=3.3
            return orig(write, elem, qnames, namespaces, short_empty_elements)
        else:
            # python <3.3
            return orig(write, elem, qnames, namespaces)

    if sys.version_info.major == 3:
        ElementTree._serialize_xml = \
            ElementTree._serialize['xml'] = _serialize_xml3
    elif sys.version_info.major == 2:
        ElementTree._serialize_xml = \
            ElementTree._serialize['xml'] = _serialize_xml2


class SuitesReportData(object):
    def __init__(self, test_suites):
        self.filename = test_suites.suites_name
        self.classname = test_suites.phasename
        self.testcases = []
        self.counts_tests = 0
        self.counts_errors = 0
        self.counts_failed = 0
        self.counts_skipped = 0

    def reset(self):
        self.testcases = []
        self.counts_tests = 0
        self.counts_errors = 0
        self.counts_failed = 0
        self.counts_skipped = 0

class TestSuites(object):
    def __init__(self, suites_name=None, phasename=None, duration = 0, directory=None):
        self.suites_name = suites_name or "first_suite" # jdbc...
        self.phasename = phasename or "None" # data_init hyper_datainit inceptor query/compare
        self.duration = duration
        self.junit_directory = directory or "./reports"

class TestCase(object):
    def __init__(self, name=None, status="skipped", duration=0, stdout=None, stderr=None):
        self.status = status
        self.name = name or "case1"
        self.duration = duration
        self.stdout = stdout
        self.stderr = stderr
        self.failmsg = "None"
        self.message = "None"
        self.file_content = ""
        self.type = "Exec Failed"

    def set_type(self, type):
        self.type = type

    def set_message(self, message):
        self.message = message

    def set_failmsg(self, failmsg):
        self.failmsg = failmsg

    def set_filecontent(self, content):
        self.file_content = content

class JUnitReporter(object):
    def __init__(self, test_suites):
        self.classname = test_suites.phasename
        self.report = SuitesReportData(test_suites)

    def reset(self, test_suites):
        self.report.reset()
        self.classname = test_suites.phasename
        self.report = SuitesReportData(test_suites)

    # -- REPORTER-API:
    def generate_reporter(self, test_suites):
        classname = self.classname
        report = self.report

        suite = ElementTree.Element(u'testsuite')
        feature_name = report.filename
        suite.set(u'name', '%s.%s' % (feature_name, classname))

        for testcase in report.testcases:
            suite.append(testcase)

        suite.set(u'tests', _text(report.counts_tests))
        suite.set(u'errors', _text(report.counts_errors))
        suite.set(u'failures', _text(report.counts_failed))
        suite.set(u'skipped', _text(report.counts_skipped))  # WAS: skips
        suite.set(u'time', _text(round(test_suites.duration, 6)))

        if not os.path.exists(test_suites.junit_directory):
            os.makedirs(test_suites.junit_directory)

        tree = ElementTreeWithCDATA(suite)
        report_dirname = test_suites.junit_directory
        report_basename = u'TESTS-%s-%s.xml' % (feature_name, classname)
        report_filename = os.path.join(report_dirname, report_basename)
        tree.write(codecs.open(report_filename, "wb"), "UTF-8")

    def _add_caseresult(self, test_case):
        report = self.report
        report.counts_tests += 1
        classname = report.classname
        feature_name = report.filename

        case = ElementTree.Element('testcase')
        case.set(u'classname', u'%s.%s' % (feature_name, classname))
        case.set(u'name', test_case.name or '')
        case.set(u'status', test_case.status)
        case.set(u'time', _text(round(test_case.duration, 6)))

        step = None
        if test_case.status in ('failed', 'errors'):
            if test_case.status == 'failed':
                report.counts_failed += 1
                element_name = 'failure'
            else:
                report.counts_errors += 1
                element_name = 'error'
            # -- COMMON-PART:
            failure = ElementTree.Element(element_name)
            failure.set(u'type', _text(test_case.type))
            failure.set(u'message', _text(test_case.message))
            text = _text(test_case.failmsg)
            failure.append(CDATA(text))
            case.append(failure)
        if test_case.status in ('skipped', 'untested'):
            report.counts_skipped += 1
            skip = ElementTree.Element(u'skipped')
            case.append(skip)

        # Create stdout section for each test case
        stdout = ElementTree.Element(u'system-out')

        # Append the captured standard output
        if test_case.stdout:
            output = _text(test_case.stdout)
            text = u'\nCaptured stdout:\n%s\n' % output
            stdout.append(CDATA(text))
            case.append(stdout)

        # Create stderr section for each test case
        if test_case.stderr:
            stderr = ElementTree.Element(u'system-err')
            output = _text(test_case.stderr)
            text = u'\nCaptured stderr:\n%s\n' % output
            stderr.append(CDATA(text))
            case.append(stderr)

        report.testcases.append(case)

    def add_caseresult(self, TestCase):
        self._add_caseresult(TestCase)

