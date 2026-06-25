Real interview question

1. Suppose 10 processes are polling.
One message arrives.
Should all 10 wake?

Usually:
No

because:
thundering herd problem

10 processes wake.
Only 1 consumes data.
9 wake up unnecessarily.


2. If two applications open same char device:
/dev/mychar

do they get separate driver instances?

Answer:
No.

They share:

global variables
global buffers
wait queues

inside the driver.

Only their:
struct file *		is different.

That is exactly why later we use:

file->private_data
container_of()

to maintain per-device/per-open context.