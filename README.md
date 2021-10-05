# containers_from_scratch
Using linux namespaces to isolate processes

# How to build
### Requirements
To build the ELF executable, you need to have a `C++` compiler available. If you're on Linux, most likely, you will have one already installed for you.

Also, you will need to have a base OS filesystem to run the container on top of. 
This repository already includes the `alpine_fs` that will start a container on `Alpine`. However, you can use your own.

### Build
To build the project, just run:
```ssh
$ g++ engine.cpp -o engine
```

# How to run
Run the following:
```ssh
$ ./engine <file_system> <command> <args>
```

### Alpine
The filesystem for Alpine is already included in the repository. To run against it:
```ssh
$ ./engine alpine_fs <command> <args>
```

##### Example
To drop a shell inside Alpine:
```ssh
$ ./engine alpine_fs /bin/sh
```

## Features
The current implementation allows you to `unshare` the following namespaces:
- `USER` namespace: creates a new user namespace where the current `uid` is mapped to `root`. This allows to have a `fakeroot` inside the container.  
- `PID` namespace: allows to see only the processes running inside the container. Thus, the container processes are isolated from the processes running on the host machine.
- `UTS` namespace: allows to isolate the host and domain names for the process. For example, we can run `$ hostname <name>` and change the hostname inside the container, however, on the host machine, the `hostname` will remain intact.
- `NS` namespace: allows to isolate the mounts. This means, we can mount different file systems on the container, however, the host will keep no track of this.

For more information, check the [namespaces](https://man7.org/linux/man-pages/man7/namespaces.7.html) documentation. 

