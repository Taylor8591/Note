cmd_/home/user/learn/A131M04/all_in_one.mod := printf '%s\n'   adc.o driver.o | awk '!x[$$0]++ { print("/home/user/learn/A131M04/"$$0) }' > /home/user/learn/A131M04/all_in_one.mod
