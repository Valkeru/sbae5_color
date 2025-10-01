# Creative SoundBlaster AE5 and AE5Plus LED driver for Linux

# WIP

## Build

### Dependencies

To build package you need dkms installed  

# Debian-based
Use docker:  
```shell
docker compose build
docker compose run --rm builder
sudo apt install ./sbae5-color-driver-dkms_0.2_all.deb
```

Next install deb file

### Arch
```shell
$ sudo pacman -Syu dkms linux-headers
$ makepkg
$ sudo pacman -U sbae5-color-dkms-0.2-1-any.pkg.tar.zst
```

## How to use

Open file `/dev/sbae5-color` and use ioctl with operations and structures described in [led_data.h](led_data.h)

Receive device data:

```c++
int fd = open("/dev/sbae5-color", O_RDONLY);

if (fd < 0) {
    std::cerr << "Failed to open device file: " << strerror(errno) << std::endl;
    return false;
}

// Device info would be written here
device_data data;

int result = ioctl(fd, IOCTL_COMMAND_READ_DEVICE_INFO, &data);
if (result < 0) {
    std::cerr << "IOCTL READ FAILED" << std::endl;
} else {
    std::cout << "Device name: " << data.name << std::endl;
    std::cout << "Device PCI location: " << data.location << std::endl;    
}

close(fd);
```

LED color setting:
```c++
// Setting white color
led_data data = {
    5,
    {255, 255, 255, 255, 255},
    {255, 255, 255, 255, 255},
    {255, 255, 255, 255, 255}
};

int fd = open("/dev/sbae5-color", O_WRONLY);
if (fd < 0) {
    std:: cerr << "Failed to open color control file: " << strerror(errno) << std::endl;

    return;
}

ioctl(fd, IOCTL_COMMAND_SET_INTERNAL_COLOR, &data);
close(fd);
```