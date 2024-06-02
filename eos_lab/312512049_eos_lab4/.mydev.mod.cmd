cmd_/home/sam/linux/312512049_eos_lab4/mydev.mod := printf '%s\n'   mydev.o | awk '!x[$$0]++ { print("/home/sam/linux/312512049_eos_lab4/"$$0) }' > /home/sam/linux/312512049_eos_lab4/mydev.mod
