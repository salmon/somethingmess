__author__ = 'bianyu'

from __future__ import absolute_import
from behave.model import reset_model, Feature, Scenario
from behave.runner import ModelRunner, Runner
from behave.parser import parse_tags
from behave.configuration import Configuration
import os, re

def convert_comma_list(text):
    text = text.strip()
    return [part.strip() for part in text.split(",")]

def convert_model_element_tags(text):
    return parse_tags(text.strip())

class Model(object):
    def __init__(self, features=None):
        self.features = features or []

class BehaveModelBuilder(object):
    REQUIRED_COLUMNS = ["statement", "name"]
    OPTIONAL_COLUMNS = ["tags"]

    def __init__(self):
        self.features = []
        self.current_feature = None
        self.current_scenario = None

    def build_feature(self, name=u"", tags=None):
        if not name:
            return None
        filename = u"%s.feature" % name
        line = 1
        feature = Feature(filename, line, u"Feature", name, tags=tags)
        self.features.append(feature)
        self.current_feature = feature
        return feature

    def build_scenario(self, name=u"", tags=None):
        if not self.current_feature:
            self.build_feature(u"feature_temp")
        filename = self.current_feature.filename
        line = self.current_feature.line + 1
        scenario = Scenario(filename, line, u"Scenario", name, tags=tags)
        self.current_feature.add_scenario(scenario)
        self.current_scenario = scenario
        return scenario

    def build_model_from_table(self, table):
        table.require_columns(self.REQUIRED_COLUMNS)
        for row_index, row in enumerate(table.rows):
            statement = row["statement"]
            name = row["name"]
            tags = row.get("tags", [])
            if tags:
                tags = convert_model_element_tags(tags)
            if statement == "Feature":
                self.build_feature(name, tags)
            elif statement == "Scenario":
                self.build_scenario(name, tags)
        return Model(self.features)

    def build_model_from_dirents(self, dirent):
        for subdir in os.listdir(dirent):
            if os.path.isdir(subdir):
                self.build_feature(subdir)
                for filename in os.listdir(subdir):
                    if os.isfile(filename) and re.match(".*.sql$", filename):
                        self.build_scenario(filename)
        return Model(self.features)

    def build_select_filename(self):
        pass

def run_model_with_cmdline(model, cmdline):
    reset_model(model.features)
    command_args = cmdline
    config = Configuration(command_args,
                           load_config=False,
                           default_format="null",
                           stdout_capture=True,
                           stderr_capture=True,
                           log_capture=False,
                           junit=True)
    model_runner = ModelRunner(config, model.features)
    runner = Runner(model_runner)
    runner.setup_paths()
    return runner.run()
