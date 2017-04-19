//
//  getentropy.c
//  Photoframe
//
//  Created by Martijn Vernooij on 28/03/2017.
//
//

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

/*
 * Get count number of random bytes
 */

int arc4random_getentropy(void *buf, size_t count)
{
	char *bufc = (char *)buf;
	FILE *fd = fopen("/dev/urandom", "r");
	if (!fd) {
		printf ("Can't open random device\n");
		exit(1);
	}
	
	size_t cursor = 0;
	size_t num_read;
	while (count) {
		clearerr(fd);
		num_read = fread(bufc + cursor, 1, count, fd);
		cursor += num_read;
		count -= num_read;
		
		if (count) {
			int e = ferror(fd);
			switch (e) {
				case EINTR:
				case EAGAIN:
					continue;
				default:
					printf ("Can't read random bytes: %d\n", e);
					exit(1);
			}
		}
	}
	
	fclose (fd);
	return 0;
}
