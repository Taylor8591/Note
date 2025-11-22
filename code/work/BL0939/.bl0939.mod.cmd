cmd_/home/user/learn/BL0939/bl0939.mod := printf '%s\n'   driver.o application.o | awk '!x[$$0]++ { print("/home/user/learn/BL0939/"$$0) }' > /home/user/learn/BL0939/bl0939.mod
