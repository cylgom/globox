#define _XOPEN_SOURCE 700
#include "nix.h"
#include "globox.h"
#include <sys/epoll.h>

void globox_epoll_init(struct globox* globox)
{
	if (globox->frame_event)
	{
		globox->epoll_fd = epoll_create(2);

		struct epoll_event ev =
		{
			EPOLLIN,
			{0},
		};

		epoll_ctl(
			globox->epoll_fd,
			EPOLL_CTL_ADD,
			globox->fd.descriptor,
			&ev);

		epoll_ctl(
			globox->epoll_fd,
			EPOLL_CTL_ADD,
			globox->fd_frame,
			&ev);
	}
	else
	{
		globox->epoll_fd = epoll_create(1);

		struct epoll_event ev =
		{
			EPOLLIN | EPOLLET,
			{0},
		};

		epoll_ctl(
			globox->epoll_fd,
			EPOLL_CTL_ADD,
			globox->fd.descriptor,
			&ev);
	}
}
