# lecture-6-7
Code for lectures 6 and 7


Worked with Kenneth

I wasn't able to test my coalescing, or splitting code, and I never got to accepting pointers on the inside of the block. Based on the error reports my free function is only working about half of the time. I could not find where it stopped working but it always works for a thousand or so ops including allocs and frees. 

One thing I noticed is my blksize shoots up randomly sometimes and I could not figure out what specifically caused this.

Realloc was also unchanged from kr because it relies on malloc and free so I did not attempt to write my own. 
