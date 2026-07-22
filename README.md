# LABscript (Outdated, will update later)
*the following is more of a tutorial/specification, will change later, this is slightly outdated to the current syntax of the language*

LABscript is a special dataflow programming language.

It is **NOT** finished (most of the features in the tutorial have not been implemented, yet), although it *can*:
1. Lex the target file(s)
2. Parse the target file(s) (Generating the AST)
3. Display the Lexed TokenArray(s) and AST(s)

## Install instructions
1. Clone the repo
2. Open the folder in a terminal
3. type and run: ```cmake -S . -B builds```
4. go to the builds folder: ```cd builds```
5. compile: ```cmake --build build```
7. go to the tests folder: ```cd ../tests```
8. execute programs like this: ```../builds/LABscript myProgram.lab``` (replace myProgram with programs of your choice)
---
### Hello World program:
```print <- “Hello World”;```

How does this work?

Well, there is special dataflow in LABScript, everything is dataflow.

```a -> b | c | d…```

The -> and | are pipes, pipes basically take the first value and pipe it to the second:

```a -> b; // a is piped to b```

There are two types of pipes:
Directional Pipes (e.g. “->” or “<-”)
Hereditary Pipes (e.g. “|”)

Directional Pipes set the direction of the dataflow, while hereditary pipes inherit the set direction. Directional pipes can be set anywhere, not just the first pipe.

```a | b -> c | d…```

or

```a | b | c <- d…```

We will be back to this shortly, but We need to introduce a new concept: objects.

```obj foo <- 1.5;```

---
### What is an object?
An object can hold any type of data, be changed to any type, essentially javascript’s var, but with a lot more features.

Adding casts during the creation of an object permanently gives it those features:

```obj n <int> <- 1;```

The following does the exact same but it’s for people who come from c-style languages:

```int n <- 1;```

An object can have multiple casts, it has more uses than what is given as an example below:

```obj n <int> <float> <- 1; // Can be a float or an int```

Any identifier can be casted like this, for example:

```if <- ||<bool> n|| | {/* This is a procedure object, it will be explained later */};```

Special types of literals can also be put inside objects, lists (tuples) and procedures (treating code as if it were data like in Lisp):

*Lists:*

```obj myList <- [1, 3.6, “Hello!”];```

Lists can be indexed just like arrays:

```print <- myList[1]; // Will output 3.6```

---
### Procedures:
	
```obj myProcedure <- {/* Some code here */};```
	
A procedure can be called like this:

```myProcedure;```

Procedures can also have parameters like such:

```obj mySecondProcedure <- ||par1|| ||par2<int>|| {/* Some Code Block */};```

A Procedure with parameters can be called like this:

```5 | “Hello” -> mySecondProcedure; // Closer to the destination means you get the earlier bucket```

These parameters are called “buckets.” When a user pipes a value into an object, it checks if it has a bucket, if it does, it puts it in the bucket, and evaluates the expression. Buckets can also be casted, and they don’t need the obj keyword like in C because basically just smart macros.

We can pipe a list:
```[“Hello”, 5] -> mySecondProcedure; // This is agnostic to the order, first in the list, means first bucket.```

But if We wanted a list as a bucket, We can do this:

```obj myThirdProcedure <- ||<list>listParameter|| {/* Code */};```

Calling it means it would not spread out the list, but give the list as a whole, and proceed filling the rest of the parameters if extra things were piped.

Let’s say We wanted to make that second and third bucket optional, We can add a question mark after it, for infinite buckets, ellipsis:

```obj myFourthProcedure <- ||int a|| ||bool b?|| ||bool c?|| {/* Code */};```

Or

```obj myFourthProcedure <- ||int a|| ||bool b…|| {/* Code */};```

There can be more arguments after an infinite one, ONLY IF the infinite bucket and the one after that has a fixed type:

```obj myFourthProcedure <- ||int a|| ||bool b…|| ||int c|| {/* Code */};```

We can call the variadic functions like this:

```myFourthProcedure <- 1 | true | false | 3;```

If the last one did not exist, and the infinite bucket was the last element, to end it, We can do this:

```myFourthProcedure <- 1 | true | false | true | ();```

We can pass arguments one by one:
```
myFourthProcedure <- 1; // Unfinished
myFourthProcedure <- true; // Unfinished
myFourthProcedure <- false; // Unfinished
myFourthProcedure <- true; // Unfinished
myFourthProcedure <- (); // Finished!
```
If it were to have optional arguments, or the last argument isn’t an infinite bucket, then once all of them are filled, it automatically goes through without having to pipe “()”.

---
### struct:
```
struct myStruct <- (
	obj n,
	int a,
);
```
As you can see, a list with the object declarations is passed.

---
### Namespace:
```
namespace myNamespace <- {
  proc myFunc <- {return secondFunc;};

  proc secondFunc <- {return 1+1};
};
```
We can call an object, and that object can call whatever objects, like a chain reaction:

```print <- myNamespace.myFunc; // prints “2”```

This is the very very basic breakdown of my language, there are many more nuances and features I haven’t covered here yet.

All of the code is just objects, namespaces, structs, and enums in the high level and also preprocessor directives like pragmasts.

All the files listed in the compilation process are compiled together and can access each others’ namespaces if explicitly listed:
```
file1.lab:
namespace myNamespace <- {
	int n <- ||a, b|| a + b;
};

file2.lab:
obj <ptr> myAdd <- import | myNamespace;

print <- myAdd | 5 | 6;
```
---

### Entry Points:
```
$myEntryPoint <- {
// Code
}
```
The Entry Point(s) give options on where to begin execution, the default entry point is the "main" entry point:
```
$main <- {
  print <- "Hello World";
}
```
You can start from any entrypoint by listing it as an argument:
```
$./file1 -myEntryPoint
```
but, doing the following:

```$./file1```
starts it in the "main" entry point
