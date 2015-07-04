from __future__ import print_function
import time
import os

from behave import when, then, step
from hamcrest import assert_that, equal_to

from mod_utils import command_shell
from self_modules.behave_model import run_model_with_cmdline


# ----------
# steps
# ----------
@step(u'I set the context parameter "{param_name}" to "{value}"')
def step_set_behave_context_parameter_to(context, param_name, value):
	setattr(context, param_name, value)


@step(u'step pass')
def step_passes(context):
	pass


# ----------
# when
# ----------
@when(u'Init Data "{shell_dir}" "{shell_file}"')
def step_run_single_sql(context, shell_file):
	# command = "transwarp -t -h {0} -database {1} -f ${dir}/$sql".format(context.hostname, context.database, sql_file)
	command = "./{0}".format(shell_file).split()
	start_time = time.time()
	context.command_result = command_shell.Command.run(command, cwd="./{0}".format(shell_file))
	end_time = time.time()
	context.command_result.show()
	print("Result cost time: {0}".format(end_time - start_time))


@when(u'Init HyperBase Data "{shell_dir}" "{shell_file}"')
def step_run_single_sql(context, shell_file):
	# command = "transwarp -t -h {0} -database {1} -f ${dir}/$sql".format(context.hostname, context.database, sql_file)
	command = "./{0}".format(shell_file).split()
	start_time = time.time()
	context.command_result = command_shell.Command.run(command, cwd="./{0}".format(shell_file))
	end_time = time.time()
	context.command_result.show()
	print("Result cost time: {0}".format(end_time - start_time))


@when(u'Init Inceptor Data "{shell_dir}" "{shell_file}"')
def step_run_single_sql(context, shell_file):
	# command = "transwarp -t -h {0} -database {1} -f ${dir}/$sql".format(context.hostname, context.database, sql_file)
	command = "./{0}".format(shell_file).split()
	start_time = time.time()
	context.command_result = command_shell.Command.run(command, cwd="./{0}".format(shell_file))
	end_time = time.time()
	context.command_result.show()
	print("Result cost time: {0}".format(end_time - start_time))


@when(u'I run single sql "{sql_file}"')
def step_run_single_sql(context, sql_file):
	# command = "transwarp -t -h {0} -database {1} -f ${dir}/$sql".format(context.hostname, context.database, sql_file)
	command = "transwarp -t -h localhost -database default -f {0}".format(sql_file).split()
	start_time = time.time()
	context.command_result = command_shell.Command.run(command)
	end_time = time.time()
	context.command_result.show()
	print("current workdir is %s" % os.getcwd())
	print("Result cost time: {0}".format(end_time - start_time))
	assert_that(context.command_result.returncode, equal_to(0), context.command_result.output)


@when(u'I run batch sql dir "{sql_dir}"')
def step_run_single_sql(context, sql_dir):
	sql_dir = ""
	model = context.behave_model
	run_model_with_cmdline(model, sql_dir)


@when(u'Test Run')
def step_test_run(context):
	command = "ls"
	start_time = time.time()
	context.command_result = command_shell.Command.run(command)
	end_time = time.time()
	context.command_result.show()


# ----------
# then
# ----------
@then(u'I differ sql results with "{answer}"')
def step_differ_results(context, answer):
	print("answer file is %s" % answer)
	print("current workdir is %s" % os.getcwd())
	with open(answer, "r") as f:
		print(u"%s" % f.read().decode('utf-8'))
	# sort_and_compare(context.command_result.stdout, f)

# Test
if __name__ == "__main__":
	print("Run here")

