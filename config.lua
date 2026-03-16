local config = {}

config.programs = {}

-- config.programs["test"] = {
-- 	cmd = "/tests",
-- 	numprocs = 8,
-- 	umask = "044",
-- 	workingdir = "/tmp",
-- 	autostart = true,
-- 	autorestart = "unexpected",
-- 	exitcodes = 0,
-- 	startretries = 3,
-- 	starttime = 5,
-- 	stopsignal = "USR1",
-- 	stoptime = 10,
-- 	stdout = "/tmp/vgsworker.stdout",
-- 	stderr = "/tmp/vgsworker.stderr",
-- }

config.programs.cat = {
	cmd = "cat",
	autostart = true,
	numprocs = 8,
	stdout = "/tmp/cat.stdout",
	stderr = "/tmp/cat.stderr",
}

config.programs.echo = {
	cmd = "echo 'Hello, World!'",
	autostart = true,
	numprocs = 8,
}

config.programs.ls = {
	cmd = "ls",
	autostart = true,
	numprocs = 8,
	stdout = "/tmp/ls.stdout",
	stderr = "/tmp/ls.stderr",
}

return config
