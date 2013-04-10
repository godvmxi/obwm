void simulate_mouse(int x,int y)
{
    int fd;
    int rel_x,rel_y;
    static struct input_event event,ev;

    fd = open("/dev/input/event2",O_RDWR);
    if(fd<0){
        debug_error("open mouse fails: fd=%d,errno=%d,%s\n",fd,errno,(char*)strerror(errno));
        goto error;
    }

    rel_x = x;
    rel_y = y;

    event.type = EV_REL;
    event.code = REL_X;
    event.value = rel_x;

    gettimeofday(&event.time,0);

    if( write(fd,&event,sizeof(event))!=sizeof(event)){
        debug_error("write rel_x fails: errno=%d,%s\n",errno,(char*)strerror(errno));
    }

    event.code = REL_Y;
    event.value = rel_y;

    gettimeofday(&event.time,0);

    if( write(fd,&event,sizeof(event))!=sizeof(event)){
        debug_error("write rel_y fails: errno=%d,%s\n",errno,(char*)strerror(errno));
    }

    /*clear*/
    write(fd,&ev,sizeof(ev));

error:
    if(fd>=0){
        close(fd);
        fd = -1;
    }
}
