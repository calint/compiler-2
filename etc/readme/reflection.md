# Reflections

## Why a language?

It started as an experiment. It began with "hello world" in assembler on Linux,
which is still in `etc/from`. I wanted to see how hard it is to build a usable
language for the application I experiment with.

I wanted a language without the mistakes that older languages have to keep. I
also wanted one without "foot guns". In a large project in production, those
can cause insomnia.

## Why should I use this language?

I would not. In my own work, I would turn on the "no undefined behavior" flag
of a performance language, such as C, C++ or Rust.

## Why no pointers or references?

The target application does not need them. It uses indexes into arrays.

I wrote some exploratory bare-metal code in Rust. In the end, the references
turned into `usize` indexes into arrays anyway. My safe C++ code ended up the
same way: pointers into a vector break when it grows, so I used indexes. Working
with values and indexes, I avoided dangling references and the other foot guns
of C++.

## Why no memory allocation?

## AI assistance

Before this project, I used AI only for small things. I would paste in a
rough function and ask it to simplify the code.

Then I asked an AI to review my code, and it suggested writing tests. I had
about 50 to 60 tests, and they only covered the happy path. The AI easily
wrote hundreds more, covering edge cases and error paths. That helped a lot.
But I also saw that I could never maintain so many tests by hand.

There is no going back.

I soon found that the AI is good at finding and fixing bugs. The changes were
small, and I reviewed each one before it was merged. I thought that was fine.

### First excitement

The AI is fast and precise. It writes utility scripts in minutes. Once I made a
syntax change to the language that meant updating hundreds of tests. The AI did
it in a couple of minutes. That alone saved me days.

As an experiment, without expecting much, I asked for a feature that would have
taken me days to implement. The AI was done in minutes. I was thrilled.

It solved a few of my problems with ease. When I asked how it knew the
solution, it said it was a common compiler technique. I have no theory of
compilers, since this is an experiment.

I still reviewed the code, but less and less closely. First I read every line.
Then I only skimmed. In the end, I cared only about the result.

The AI was good at fitting new features into the existing code. But I kept
adding feature after feature, and I cared less about the code than about the
result.

Then it happened.

### The slop

Every time I thought I was done, there was one more thing. Finally I stopped
adding features and declared the project finished. Then I tried to read the
code. First I scrolled through it. Then I tried to follow it line by line. It
was hard. The code was tangled and difficult to follow.

### The realization

At some point the slop got so bad that I spent a lot of time just trying to
understand the code. It had been hacked together one feature at a time. Each
feature was forced to fit into the existing framework.

I realized that the design had to be straightened out, or the project would
collapse. The features had to fit together cleanly, like a crossword puzzle and
a solved Sudoku at the same time.

### The steering and refactoring

So I stopped all new features. I told the AI to review everything and to
refactor where needed. The goal was a design that a human reviewer could
understand easily, with simplicity first.

I saw the code as a set of equations. They solved the problem, but nobody had
simplified them yet.

### The swarm

I have now spent as much time untangling the mess as I spent directing the AI
to build the features.

Right now I run a swarm of AI agents. They look for bugs and refactor the design
until a human can read it easily.

### The sense of losing touch with the code

The pre AI code and structure is visible as the skeleton of the project. I do
no longer know the source line by line like the back of my hand. I feel more
like a project director with coding experience guiding AI to a finished product.

### Conclusion

AI is a faithful and worthy partner in developing and maintaining a project.
