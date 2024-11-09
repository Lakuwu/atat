# atat

**atat** is a simple program for inserting the contents of one text file within another.

## Examples
If the content of `input.txt` is the following:

```
Hello!
@@insert example.txt@@
```
and `example.txt`:
```
This is an example.
```
running `atat input.txt output.txt` will produce the following `output.txt`:
```
Hello!
This is an example.
```
---
Instead, if `input.txt` is the following:
```
Hello @@arg 0@@! Nice to meet you.
```
and we run `atat input.txt output.txt Laku` the resulting `output.txt` will be:
```
Hello Laku! Nice to meet you.
```
---
You can also pass the filename as an argument; consider the following `mail.txt`:
```
Dear @@arg 0@@,

@@insert_arg 1@@

Sincerely,
@@arg 2@@
```
and `message.txt`:
```
You are awesome! :)
```
running `atat mail.txt output.txt Wayne message.txt Laku` will produce the following `output.txt`:
```
Dear Wayne,

You are awesome! :)

Sincerely,
Laku
```
