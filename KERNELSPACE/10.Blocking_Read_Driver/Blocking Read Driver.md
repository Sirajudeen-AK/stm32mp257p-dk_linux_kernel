Example:

GPS Driver
fd = open("/dev/gps", O_RDONLY);

read(fd, buf, sizeof(buf));

No data yet.

Process sleeps.

GPS packet arrives.

ISR:

gps_data_available = 1;

wake_up_interruptible(&gps_wq);

Sleeping read wakes.

Returns data.

Flow

				User:

				read()
				↓
				Kernel:

				my_read()
				↓
				No data
				↓
				Sleep

				wait_event_interruptible()
				↓
				ISR / write()

				wake_up_interruptible()
				↓
				read() continues
				↓
				copy_to_user()
				↓
				return


Interview Question
Difference between poll and blocking read?

	Blocking Read

		read()

		sleeps.

		One syscall.

		Simple.



	Poll

		poll()
		read()

		Two syscalls.

		Supports many fds.

		Used by event-driven applications.



Interview Questions

Q: Why wait_event_interruptible()?

Process sleeps instead of wasting CPU in a loop.

Q: What wakes it?

wake_up_interruptible(&my_wq);

Q: Why not while(1)?

Busy waiting wastes CPU.

Q: Difference between poll() and blocking read()?

Blocking read → one fd, simple
poll/epoll → many fds, event-driven applications