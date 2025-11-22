cmd_/home/user/learn/PWM/Module.symvers :=  sed 's/ko$$/o/'  /home/user/learn/PWM/modules.order | scripts/mod/modpost       -o /home/user/learn/PWM/Module.symvers -e -i Module.symvers -T - 
