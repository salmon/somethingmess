import difflib, os, sys
import StringIO

class SortDiff(object):
    def __init__(self):
        pass

    @staticmethod
    def filter():
        pass

    @staticmethod
    def sort_differ_buffers(result, origin_filename):

        if not os.path.isfile(origin_filename):
            print ("%s" % origin_filename)
            return "Origin filename not exist"
        with open(origin_filename, "r") as f:
            lines_ans = f.readlines()
            lines_ans.sort()
        s = StringIO.StringIO(result)
        lines_res = s.readlines()
        lines_res.sort()
        diff = difflib.context_diff(lines_res, lines_ans)
        diffs = "".join(list(diff))

        return diffs

    @staticmethod
    def filter(self):
        pass

if __name__ == "__main__":
    with open("/home/bianyu/PycharmProjects/diff_files/q_3_iud.x1", "r") as f:
        SortDiff.sort_differ_buffers(f.read(), "/home/bianyu/PycharmProjects/diff_files/q_3_iud.x2")


