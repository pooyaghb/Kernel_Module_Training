#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

int main(void)
{
	int fd = open("/dev/spich_dev", O_RDWR);
	if(fd < 0) return -1;

	uint16_t c = 0x8000 ;
	write(fd, &c, sizeof(c));
	read(fd, &c, sizeof(c));
	printf("0x%02x\n", c & 0x00FF);
	close(fd);

	return 0;
}
