# Reflections

## Why a language?

It started as an experiment. It began with "hello world" in assembly on Linux,
which is still in `etc/from`. I wanted to see how hard it is to build a usable
language for the application I experiment with. I worked on it on and off,
returning to it from time to time.

I wanted a language without the mistakes that older languages have to keep. I
also wanted one without "foot guns". In a large project in production, those
can cause insomnia. The certainty that a program does what is expected, or
exits with a failure, is worth the cost in performance.

## Why should I use this language?

Don't. Instead, I would turn on the "no undefined behavior" flag of a
performance language, such as C, C++ or Rust.

## Why no pointers or references?

The target application does not need them. It uses indexes into arrays.

I wrote some exploratory bare-metal code in Rust. In the end, the references
turned into `usize` indexes into arrays anyway. My safe C++ code ended up the
same way: pointers into a vector break when it grows, so I used indexes.

## Why no memory allocation?

## AI assistance

Before this project, I used AI only for small things. I would paste in a
rough function and ask it to simplify the code.

Then I asked an AI to review my code, and it suggested writing tests. I had
about 50 to 60 tests, and they only covered the happy path. The AI easily
wrote hundreds more, covering edge cases and error paths. That helped a lot.
But I also saw that I could never maintain so many tests by hand.

I soon found that the AI is good at finding and fixing bugs. The changes were
small, and I reviewed each one line by line before it was merged.

### The exuberance

The AI is fast and precise. It writes utility scripts in seconds. Once I made a
syntax change to the language that meant updating hundreds of tests. The AI did
it in a couple of minutes. That alone saved me days.

As an experiment, without expecting much, I asked for a feature that would have
taken me days to implement. The AI was done in minutes. I was thrilled.

It also solved a few of my problems with ease. When I asked how it knew the
solution, it said it was a common compiler technique. In this case, the problem
was how to handle end-of-line comments: make them part of the whitespace that
belongs to the token. I thought that was clever.

At times it drifted into a bad solution and had to be corrected and steered
onto a different line of thought.

I have no background in compiler theory; this is an experiment. That shows
in the arithmetic and boolean expressions, which are handled without an abstract
syntax tree.

I still reviewed the code, but less and less closely. First I read every line.
Then I only skimmed. In the end, I just committed.

The AI was good at fitting new features into the existing code. But in my
exuberance I kept adding feature after feature, and I cared less about the code
than about the result.

Then it happened.

### The slop

Every time I thought I was done, there was one more thing. Finally I stopped
adding features and declared the project finished. Then I tried to read the
AI-generated code. First I scrolled through it to get a sense of its size. At a
skim, it looked manageable. Then I tried to follow it line by line. It was
hard. The code was tangled and difficult to follow, clearly wedged into the
existing framework.

### The realization

At some point the slop got so bad that I spent a lot of time just trying to
understand the code. It had been hacked together one feature at a time.

I realized that the design had to be straightened out, or the project would
collapse under its own complexity.

The features had to fit together cleanly, like a crossword puzzle and a solved
Sudoku at the same time.

### The steering and refactoring

So I stopped all new features. I told the AI to review everything and to
refactor where needed, until it found no more issues. The goal was a design
that a human reviewer could understand easily, with simplicity as the priority.

I saw the code as a set of equations. They solved the problem, but nobody had
simplified them yet.

### The swarm

I have now spent as much time untangling the mess as I spent directing the AI
to build the features.

Right now I run a swarm of AI agents. They look for bugs and refactor the design
until a human can read it easily.

The amount of bugs, unresolved edge cases and inconsistencies it finds is scary.
This is a relatively simple language. Are complex languages written in a
flawless way by superior engineers or is AI going to scare them?

### The sense of losing touch with the code

The pre-AI code and structure are still visible as the skeleton of the project.
I no longer know the source line by line, like the back of my hand. I feel more
like a project director with coding experience, guiding the AI toward a
finished product.

### Conclusion

AI is a faithful and worthy partner in developing and maintaining a project.

There is no going back.
