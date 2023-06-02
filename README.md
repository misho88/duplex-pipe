# duplex-pipe
bidirection communication between programs

# description

`duplex-pipe` sets up two pipes, `std` and `rev`, where `std` works like in normal shell piping wherein `stdout` of the data-producing application connects to `stdin` of the data-consuming application. `rev` goes the other way, so that the consumer can control the producer. The applications in question have to support this, of course.

# examples

Just two applications, `2nd` controls `1st`. Note that, with the default arguments, `2nd`'s standard output points back at `1st`, so I'm using standard error to print:

```
$ cat 1st
#!/usr/bin/env python
from datetime import datetime
while True:
    print(datetime.now().isoformat(), flush=True)
    if input() == 'stop':
        break
$ cat 2nd
#!/usr/bin/env python
from sys import stdin, stdout, stderr
for i, line in zip(range(3), stdin):
    print(*line.strip().split('T'), sep='\n', end='\n\n', file=stderr)
    print('stop' if i == 2 else '', file=stdout, flush=True)
$ duplex-pipe ./1st '' ./2nd
2023-06-01
15:50:13.988538

2023-06-01
15:50:13.988593

2023-06-01
15:50:13.988616

```

Chaining more applications, `third` controls `second` which controls `first`. We use FDs `3` and `4` for the reverse (control) pipes. Note that I'm using higher-numbered FDs that the programs need to be aware of:

```
$ cat first
#!/usr/bin/env python
from datetime import datetime
ctlin = open(3, 'r')
while True:
    print(datetime.now().isoformat(), flush=True)
    command = ctlin.readline().removesuffix('\n')
    if command == 'stop':
        break
$ cat second
#!/usr/bin/env python
from sys import stdin
ctlin, ctlout = open(3, 'r'), open(4, 'w')
for line in stdin:
    line = line.removesuffix('\n')
    for sub in line.split('T'):
        print(sub)
    print(flush=True)
    command = ctlin.readline().removesuffix('\n')
    print(command, file=ctlout, flush=True)
$ cat third
#!/usr/bin/env python
from sys import stdin
ctlout = open(4, 'w')
for i, line in zip(range(3), stdin):
    line = line.removesuffix('\n')
    for sub in line.split('-'):
        print(sub)
    line = stdin.readline().removesuffix('\n')
    for sub in line.split(':'):
        print(sub)
    line = stdin.readline().removesuffix('\n')
    assert line == ''
    print(flush=True)
    print('stop' if i == 2 else '', file=ctlout, flush=True)
$ duplex-pipe -I 3 -O 4 ./first '' duplex-pipe -I 3 -O 4 ./second '' ./third
2023
06
01
16
03
51.068216

2023
06
01
16
03
51.068327

2023
06
01
16
03
51.068383

```

Chaining more applications, `third` controls both `first` and `second` directly:

```
$ cat first
#!/usr/bin/env python
from datetime import datetime
ctlin = open(3, 'r')
while True:
    print(datetime.now().isoformat(), flush=True)
    command = ctlin.readline().removesuffix('\n')
    if command == 'stop':
        break
$ cat second
#!/usr/bin/env python
from sys import stdin, stderr
ctlin = open(3, 'r')
for line in stdin:
    line = line.removesuffix('\n')
    for sub in line.split('T'):
        print(sub)
    print(flush=True)
    command = ctlin.readline().removesuffix('\n')
    if command == 'more':
        print('second: expecting more from first', file=stderr)
$ cat third
#!/usr/bin/env python
from sys import stdin
ctlout1, ctlout2 = open(4, 'w'), open(5, 'w')
for i, line in zip(range(3), stdin):
    line = line.removesuffix('\n')
    for sub in line.split('-'):
        print(sub)
    line = stdin.readline().removesuffix('\n')
    for sub in line.split(':'):
        print(sub)
    line = stdin.readline().removesuffix('\n')
    assert line == ''
    print(flush=True)
    print('stop' if i == 2 else '', file=ctlout1, flush=True)
    print('more' if i != 2 else '', file=ctlout2, flush=True)
$ duplex-pipe -I 3 -O 4 ./first '' duplex-pipe -I 3 -O 5 ./second '' ./third
2023
06
01
16
14
48.500630

second: expecting more from first
2023
06
01
16
14
48.500730

second: expecting more from first
2023
06
01
16
14
48.500799
```
