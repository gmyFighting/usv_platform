# 项目架构组成
# Logger Sensor_manager INS LED KEY Timer Watchdog...

sig_handler
    sem_post

sensor_loop
    cnt++
    
thread:
    sensor_collect_func
        timer_create(sig_handler)
        while(1)
            if(sem_wait) 
                sensor_loop()

