cmd_/home/user/learn/BL0939/Module.symvers :=  sed 's/ko$$/o/'  /home/user/learn/BL0939/modules.order | scripts/mod/modpost       -o /home/user/learn/BL0939/Module.symvers -e -i Module.symvers -T - 
