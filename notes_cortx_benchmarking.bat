# Generate lease 
    links:
        https://www.chameleoncloud.org/hardware/
    Support infiniband :
        "$node_type", "compute_skylake"
    Max lease: 
        7 days
    Floating IP needed 
        1 
# Launching an instance
    # In the sidebar, click Compute, then click Instances
    # Click on the Launch Instance
        # pick the correct reservation 
        # count = 1 (for singlenode)
        # Image: CC-CentOS7-2003    # need 1062
    # Choose the ssh key
        # public key
        ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQDBotmMy87bcgiSSEjmF7ONIbGDTDgzLG+nBUPmO3l51A5XYFBR7EVq7ezb4cMN7U01XjC2yuDodyuWYxoQkUjiDo+uoDLOqiyE5vtC9OGLIOowOMn2z9VnIR14BZ/ZZrdVJhNTIvcg1x7CIZ+dJoYmmPrkTYOclSY87HEd6DU9Sbh+S8PDXaW32MFKLaQFm9u//BWvP6s1MBntL/WuzCSIa6OaxcO2L+dEr2+yqr3hW48vy73/4UNljKW3p10+7icf+Svh4tcnk9EC1l3mGAPdTsvHHaD4wTafw/1JCO190qg9mvJLeW9XMHQ9YjqE3iAg8vn9F7Y6XVHEtj9K/t6F fandi@fandi-cortx.novalocal
# Allocate floating IPs
    # Book the IP interface
        # Click "Network -> Floating IPs -> Allocate IP To Project"
        # Write description
        # Click "Allocate IP"

    # click "attach interface"
        # Click "Network -> Floating IPs"

    # ony 6 available public interfaces
    # wait a few minutes
# SSH to the node!
    # https://chameleoncloud.readthedocs.io/en/latest/getting-started/index.html#step-3-start-using-chameleon

    ssh cc@129.114.109.205

00. Preparation [Login as "cc"]

    # Use cc user!!
    ssh cc@129.114.109.205

    # Setup disk 
        # check if there is already mounted disk
        df -H
            # /dev/sda1       251G  2.8G  248G   2% /
            # should be enough

    # Setup user fandi 
    sudo adduser fandi
    sudo usermod -aG wheel fandi
    sudo su 
    cp -r /home/cc/.ssh /home/fandi
    chmod 700  /home/fandi/.ssh
    chmod 644  /home/fandi/.ssh/authorized_keys
    chown fandi  /home/fandi/.ssh
    chown fandi  /home/fandi/.ssh/authorized_keys
    echo "fandi ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers.d/90-cloud-init-users
    exit
    exit

0. Setup zsh [Login on "fandi"]

    ssh -o TCPKeepAlive=yes -o ServerAliveCountMax=900000 -o ServerAliveInterval=30 129.114.109.205 

        sudo su
        yum install zsh -y
        chsh -s /bin/zsh root
        chsh -s /bin/zsh fandi
        exit
        which zsh
        echo $SHELL

        sudo yum install wget git vim zsh -y

        printf 'Y' | sh -c "$(wget -O- https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)"

        /bin/cp ~/.oh-my-zsh/templates/zshrc.zsh-template ~/.zshrc
        sudo sed -i 's|home/fandi:/bin/bash|home/fandi:/bin/zsh|g' /etc/passwd
        sudo sed -i 's|ZSH_THEME="robbyrussell"|ZSH_THEME="risto"|g' ~/.zshrc
        zsh
        # sudo source ~/.zshrc

1. Downgrade Centos 

    sudo su 
    # yum -y update # DO NOT RUN THIS (the kernel will be updated!)

    mkdir /mnt
    mkdir /mnt/extra
    sudo chown fandi -R /mnt

    cd /mnt/extra

    wget https://buildlogs.centos.org/c7.1908.00.x86_64/kernel/20190808101829/3.10.0-1062.el7.x86_64/kernel-3.10.0-1062.el7.x86_64.rpm
    wget https://buildlogs.centos.org/c7.1908.00.x86_64/kernel/20190808101829/3.10.0-1062.el7.x86_64/kernel-tools-3.10.0-1062.el7.x86_64.rpm
    wget https://buildlogs.centos.org/c7.1908.00.x86_64/kernel/20190808101829/3.10.0-1062.el7.x86_64/kernel-tools-libs-3.10.0-1062.el7.x86_64.rpm
    wget https://buildlogs.centos.org/c7.1908.00.x86_64/kernel/20190808101829/3.10.0-1062.el7.x86_64/kernel-headers-3.10.0-1062.el7.x86_64.rpm

    rpm -ivh --force kernel-tools-libs-3.10.0-1062.el7.x86_64.rpm
    rpm -ivh --force kernel-tools-3.10.0-1062.el7.x86_64.rpm
    rpm -ivh --force kernel-headers-3.10.0-1062.el7.x86_64.rpm
    rpm -ivh --force kernel-3.10.0-1062.el7.x86_64.rpm

    awk -F\' '$1=="menuentry " {print i++ " : " $2}' /etc/grub2.cfg

    grub2-set-default 1
    echo "CentOS Linux release 7.7.1908 (Core)" > /etc/redhat-release
    cat /etc/redhat-release
    uname -r
    reboot

        # 4 minutes
        https://chi.tacc.chameleoncloud.org/project/instances/
        # got to "console", wait till screen goes BLACK!!  


2. Download Motr
    
    sudo su
    
    yum group install -y "Development Tools"
    # yum install -y python3
    yum install -y python-devel
    yum install -y ansible
    
    curl https://bootstrap.pypa.io/2.7/get-pip.py  -o get-pip.py
    pip install --upgrade "pip < 21.0"

    #pip install --upgrade pip # DON'T RUN
    exit
    sudo pip install --target=/usr/lib64/python2.7/site-packages ipaddress
    
    # Force ansible to use Python2
    sudo su
    echo "all:" >> /etc/ansible/hosts
    echo "  ansible_python_interpreter: \"/usr/bin/python2\"" >> /etc/ansible/hosts
    exit

    cd /mnt/extra
    wget https://buildlogs.centos.org/c7.1908.00.x86_64/kernel/20190808101829/3.10.0-1062.el7.x86_64/kernel-devel-3.10.0-1062.el7.x86_64.rpm
    sudo rpm -ivh --force kernel-devel-3.10.0-1062.el7.x86_64.rpm

    # Clone cortx github
    cd /mnt/extra
    git config --global user.email "fandi.z.w@gmail.com"
    git config --global user.name "fandiazam"

    git clone https://github.com/fandiazam/cortx-motr-new.git
    mv cortx-motr-new cortx-motr

    cd /mnt/extra/cortx-motr/extra-libs/
    git clone https://github.com/fandiazam/galois.git

    cd /mnt/extra/cortx-motr/
    sudo ./scripts/install-build-deps
    
    Output :
        PLAY RECAP *********************************************************************
        localhost                  : ok=78   changed=51   unreachable=0    failed=0    skipped=8    rescued=0    ignored=2

    tput bel; sleep 2; tput bel; sleep 2; tput bel ; sleep 2; tput bel; sleep 2; tput bel
    
    sleep 4; sudo reboot &
    exit
        # 4 minutes
        https://chi.tacc.chameleoncloud.org/project/instances/91a2a0f3-d5c2-4ecb-98f8-baefbffa2029/
        # got to console, wait till screen goes BLACK and user ssh sign ">>"!!    

3. ACTIVATE the lnet [AFTER EACH REBOOT]
    
    "FIND interface for LNET"
    sudo ifconfig | grep MULTICAST
    # use the ethXX address to initiate lnetctl

    sudo systemctl stop lnet
    sudo systemctl start lnet
    sudo lnetctl net add --net tcp --if eth0

    sudo modprobe lnet
    sudo lctl list_nids

4. Compile Motr

    # Disable any changes to example1.c FIRST!!
    # RUN ONLY ONCE!
        cd /mnt/extra/
        if [ ! -d  /mnt/extra/tmp-examples ]; then
            mv /mnt/extra/cortx-motr/motr/examples /mnt/extra/tmp-examples
            mkdir /mnt/extra/cortx-motr/motr/examples
            cp /mnt/extra/tmp-examples/example1.c /mnt/extra/cortx-motr/motr/examples/example1.c
            cp /mnt/extra/tmp-examples/setup_a_running_motr_system.sh /mnt/extra/cortx-motr/motr/examples/
            cp /mnt/extra/tmp-examples/Makefile.sub /mnt/extra/cortx-motr/motr/examples/
            cp /mnt/extra/tmp-examples/example_mt_bench_disk.c /mnt/extra/cortx-motr/motr/examples/example_mt_bench_disk.c
            cp /mnt/extra/tmp-examples/example_mt_bench_memory.c /mnt/extra/cortx-motr/motr/examples/example_mt_bench_memory.c
            cp /mnt/extra/tmp-examples/parser.py /mnt/extra/cortx-motr/motr/examples/parser.py
            cp -R /mnt/extra/tmp-examples/.* /mnt/extra/cortx-motr/motr/examples/
            cp /mnt/extra/tmp-examples/example2.c /mnt/extra/cortx-motr/motr/examples/
            cp /mnt/extra/tmp-examples/.gitignore /mnt/extra/cortx-motr/motr/examples/.gitignore
        fi

    # put back 
    # rm -rf /mnt/extra/cortx-motr/motr/examples
    # mv /mnt/extra/tmp-examples /mnt/extra/cortx-motr/motr/examples
    
    sudo chown $USER -R /mnt
    cd /mnt/extra/cortx-motr/

    # For benchmarking, 
    ./scripts/m0 rebuild

====================================================================
      Run Native Client Motr (simple scripts)
====================================================================

1. SINGLE NODE

    # Run the cortx-motr  (NO NEED After each compile!)

        cd /mnt/extra/cortx-motr/
        sudo ./motr/examples/setup_a_running_motr_system.sh


2. OS benchmarking
    2.1 On-disk & In-memory
        #Generate large file/dataset ->10 GB
            #dd if=/dev/urandom of=10GB.txt bs=1073741824 count=10
            while true;do head /dev/urandom | tr -dc A-Za-z0-9;done | head -c 1073741824 | tee  1GB.txt

            for i in {1..10}; 
                do cat 1GB.txt >> 10GB.txt
            done
            rm -f 1GB.txt
        #RUN
        cd mnt/extra/cortx-motr/OS_benchmarking/

                                                            
        gcc -o OS_bench OS_bench.c;

        #[object file] [blocksize: 1 -> 4096 B] [dataset name] [N_Request]
        sudo ./OS_bench 1 10GB.txt 1000


3. Cortx Benchmarking :

    3.0 Follow step 4.1 then 4.3 for setup and rebuild

    3.1 On-disk benchmarking
        cd /mnt/extra/cortx-motr/motr/examples
        zsh
        sudo su
        LD_LIBRARY_PATH="/mnt/extra/cortx-motr/motr/.libs/"
        export LD_LIBRARY_PATH
        gcc -I/mnt/extra/cortx-motr -I/mnt/extra/cortx-motr/extra-libs/galois/include \
            -DM0_EXTERN=extern -DM0_INTERNAL= -Wno-attributes               \
            -L/mnt/extra/cortx-motr/motr/.libs -lmotr  -lpthread            \
            example_mt_bench_disk.c -o example_mt_bench_disk
        
        #Write Request 
            #below code write 40 MB for test only; if real benchmarking, it needs 10 GB (by changing "0..9" to "0..2621440"-> takes > 6 hours)
            
            for i in {0..9}; 
            do 
                z=$((90000000+i*1000))
                cmd=$(./example_mt_bench_disk 10.52.3.173@tcp:12345:34:1 10.52.3.173@tcp:12345:33:1000  "<0x7000000000000001:0>" "<0x7200000000000001:64>" $z 1 0 0 1 1000)
                echo $cmd
            done

            #Read request
                                    HA_ADDR LOCAL_ADDR Profile_fid Process_fid(m0_client's fid) obj_id write read delete layout_id n_thread_sema
            ./example_mt_bench_disk 10.52.3.173@tcp:12345:34:1 10.52.3.173@tcp:12345:33:1000  "<0x7000000000000001:0>" "<0x7200000000000001:64>" 90000000 0 1 0 1 1

    3.2 In-memory benchmarking
        cd /mnt/extra/cortx-motr/motr/examples
        zsh
        sudo su
        LD_LIBRARY_PATH="/mnt/extra/cortx-motr/motr/.libs/"
        export LD_LIBRARY_PATH
        gcc -I/mnt/extra/cortx-motr -I/mnt/extra/cortx-motr/extra-libs/galois/include \
            -DM0_EXTERN=extern -DM0_INTERNAL= -Wno-attributes               \
            -L/mnt/extra/cortx-motr/motr/.libs -lmotr  -lpthread            \
            example_mt_bench_memory.c -o example_mt_bench_memory

        #Write Request
        ./example_mt_bench_investigate_memory 10.52.3.173@tcp:12345:34:1 10.52.3.173@tcp:12345:33:1000  "<0x7000000000000001:0>" "<0x7200000000000001:64>" 12349860 1 0 0 1 1
                    
        vim example_mt_bench_investigate.c 
        #change N_THREAD = 10000

        #Read request
        ./example_mt_bench_investigate_memory 10.52.3.173@tcp:12345:34:1 10.52.3.173@tcp:12345:33:1000  "<0x7000000000000001:0>" "<0x7200000000000001:64>" 12349860 0 1 0 1 1

4. Investigate the Top-10 Bottlenecks (See these functions in "CORTX:Benchmarking" spreadsheet)
    4.1 Setup 
        cd /mnt/extra/cortx-motr/

        ./configure --disable-expensive-checks
        ./configure --disable-immediate-trace
        ./configure --disable-dev-mode
        
        #set highest trace level that will be enabled.
        ./configure --with-trace-max-level=M0_INFO 

    4.2 Get per function tracing
        #For example m0_op_wait() in motr/client.c. Add below code to the client.c
        
            #header
                #include "lib/time.h"

            #inside function 
                m0_time_t         result1;
                m0_time_t         result2;

                result1 = m0_time_now(); //record the entry time

                //Function

                result2 = m0_time_now(); //record the leave time
                M0_LOG(M0_INFO, "[m0_op_wait] %"PRIu64" %"PRIu64, resultt1, resultt2); //print to log file 

        Note : Don't forget to clear these codes after completing the investigation 

    4.3 Rebuild
        cd /mnt/extra/cortx-motr/
        make 

    4.4 follow step 3.2
        - On read request, just use 100x requests (by changing N_THREAD). 
        - After read request, it will generate m0trace.[PID] 

    4.5 Then, use a parser code to read the log file and get the latency.
         
        mkdir /var/log/motr #[JUST ONCE]

        #Convert the trace from YAML to txt
            sudo su
            cd /mnt/extra/cortx-motr/
            ./utils/trace/m0tracedump -i /mnt/extra/cortx-motr/motr/examples/m0trace.[PID] -o /var/log/motr/m0trace.[PID].yml
            ./utils/trace/m0trace -i /var/log/motr/m0trace.[PID].yml -o /mnt/extra/cortx-motr/motr/examples/m0trace.[PID].txt

        #read the log file. Then get the latency and function occurrence. Making sure that the function occurrence (#Call/request) is linear to the number of request
            
            python parser.py m0_op_wait /mnt/extra/cortx-motr/m0trace.[PID].txt

    4.6 Validate per function read-path. 
        - Add sleep (1 s) on function XXX
            
            #include "lib/time.h"
            m0_nanosleep(10000000000, NULL);

        - Rebuild -> follow step 4.3

        - Then read 10 times -> follow step 3.2 by changing N_THREAD = 10 
          See the average read latency also increased by 1 s.
        - Note : Don't forget to clear the sleep code after completing the per function investigation 





0. Update to Github 
     cd ~/Documents/GIK/cortx-motr-new/
     git add . #(untuk semua folder)
     git commit -m "add cortx-motr"
     git remote add origin https://github.com/fandiazam/cortx-motr-new.git #git remote remove origin 
     git push -u origin main
     git push origin main 





scp /Users/fandi/Documents/GIK/cortx-motr-new/OS_benchmarking/OS_bench.c 129.114.109.247:/mnt/extra/OS_bench.c




