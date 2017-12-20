# Parallel optimizer for Gadget

Paramin runs the optimisation routines in parallel across a network of
processors which will (hopefully) result in a much faster optimisation
run.  The current version of paramin is implemented with MPI, Message
Passing Interface, which handles all the message passing between the
master process, paramin, and the slave processes, which are Gadget
simulation runs.  The setup is very similar to a normal Gadget run,
with all the Gadget input files are the same, it's only the
optimisation execution that differs.

## Installation

In order to begin one must first download and install a version of
MPI.  This version of paramin is made with an implementation of MPI-2
called OPEN-MPI, one can download the MPI library and a wrapper
compiler called `mpic++` from their [website](www.open-mpi.org).  Now
before trying to run paramin make sure you have the newest version of
Gadget downloaded from
[hafro/gadget](https://github.com/Hafro/gadget/). Now you should open
a console window and navigate to the folder where you saved Gadget.
Now you can simply type

```
make gadgetpara
```

in the console and the program should be compiled and you get a gadget
executable that is network aware (by default it is called
`gadget-para`). In addition to compiling a network aware gadget
executable you need to compile a few Gadget objects into a library
which paramin depends on. To do that you need to type the following in
the console:

```
make libgadgetinput.a
```

Now you are ready to start compiling paramin.  Download the newest
version of paramin and open the paramin folder in the console.  You
need to edit the Makefile and add the path to the directory where you
installed Gadget as described above if the folder is not placed in the
same directory as the source folder.


## Running paramin

To be able to run paramin it's ideal to add the path of the two
executables, gadget-para and paramin to the global variable PATH.  If
you're running a bash console you can edit the `.bashrc` to add it,
you can also copy the executables to the directory of PATH, but you
probably need root access to do that.  Now you can call the functions
from the console.  Here is an example of a typical paramin run:

```
mpirun -np 1 paramin -i <filename> -opt <filename> -network <filename> 
    -o <filename> -function gadget-PARA -s -n -i <filename> > <filename>
```

Now because paramin is implemented with MPI we need to call mpirun
when we run it and the `np` switch tells the server how many instances
of paramin should be run.  Now there are several files we need to call
and a few switches we need. The `-i <filename>` switch tells paramin
which file holds the parameters and their initial values, this file is
the same as the params.in file used in Gadget. The `-i` switch with
the same `<filename>` has to be on Gadget as well.

Next we need to specify the optimization parameters with the `-opt
<filename>` switch, the preceding file contains the information of
which type of optimization run to perform for more information see
Parameter Files, it's the same format as the optimization file for
Gadget.

The next switch tells paramin how many subprocesses paramin will
spawn, this switch is not in Gadget.  Note that having at least 8
subprocesses will greatly improve the time of the run, but there is no
need to let the number of subprocesses exceed the number of
parameters, optimally it should be best to have as many subprocesses
as the number of parameters, but then one should have access to at
least that many processing cores.  A sample file which tells paramin
to spawn 30 subprocesses looks like this:

```
;
;Sample network file
;
numproc    30
```

The `-o <filename>` switch specifies which file to output the optimized parameters.  This file will also contain information about the optimization run, how much overall time was taken and how much time was taken for each optimizing algorithm.  The format of this outputted file is the same as the one used as input, so the output can be used as a starting point for later runs.

The `-function gadget-PARA` -switch tells paramin the name of the executable it will spawn as it's subprocess, in this case it's the `gadget-PARA` executable we made earlier.  The user can also make his own function to optimize with paramin.  Now Gadget also needs parameters, we pass to it the `-s` and `-n` switch to make it go to network mode, then Gadget knows it's running with paramin.  Finally we can add \texttt{> <filename>} to specify a file for the output of the run, this is not necessary but recommended.

## Additional information
If you're running a potentially very long run on a remote host it's recommended to use the `screen` utility, for more information type `man screen` in the console.  It allows you to detach the session so it lives after you log out of your SSH session.  In short you can type `screen` at the console to open a new screen session.  Now run the script or program you want to run and type `ctrl-a d`, and you detach the session.  Now you can safely exit from the remote host. To retrieve your session on your next login simply type `screen -r` and you enter the screen session, then you can close it by typing `exit` in the console. Another option is to run the desired command with the `nohup` prefix, it makes the command immune to hangups.

As stated earlier paramin can be used to optimize almost any function which can be defined within the C++ language.  The Rosenbrock function is often used to test nonlinear optimization algorithms because it's quite challenging for methods which do not rely on the gradient of the function.  A sample rosenbrock function and instruction on how to compile and run it should be available on the github website under paramin. This function can be used as a template for the optimization of any function definable in the C++ language, future work will be to add a parser and an R interface, so paramin can be called from within an R session using an R defined function.

Paramin was originally written with PVM, but since PVM is no longer supported it was migrated to MPI.  More information on the migration process from PVM to MPI can be found on the MRI website, there one can also find study on the improvement in time relative to the number of processes spawned for each optimization routine.
