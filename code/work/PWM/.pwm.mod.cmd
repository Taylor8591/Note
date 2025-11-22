cmd_/home/user/learn/PWM/pwm.mod := printf '%s\n'   pwm.o | awk '!x[$$0]++ { print("/home/user/learn/PWM/"$$0) }' > /home/user/learn/PWM/pwm.mod
