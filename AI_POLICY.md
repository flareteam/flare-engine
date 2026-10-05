# AI Usage Policy

## Preface

At the time of writing this, Flare has received and merged a handful of
AI-assisted contributions. These have been minor corrections to build scripts
and documentation. One might argue that this means the flood gates have already
opened. So as the project's lead (and currently sole) maintainer, I (@dorkster)
feel now is the best time lay out our rules with regards to AI usage. Keep in
mind that this document is subject to change as the culture around AI evolves.

Before continuing, I would however like to express my personal thoughts on AI
usage. The extent of my AI usage is as a glorified search engine, and I have
never used it to assist with programming, art, or game design. I believe that
all three of these creative endeavors (yes, I'm counting programming as
a creative endeavor) deserve to be experienced as the technical, skilled
processes that they are. I would urge anyone that has not earnestly attempted
these the traditional way to avoid reaching for AI in the first place. The
satisfaction is usually worth more than the end result.

Much of the following has been lifted verbatim from [Ghostty's AI
policy](https://github.com/ghostty-org/ghostty/blob/main/AI_POLICY.md), which
I've found to be pragmatic with its focus on maintaining the human element.

## Rules

The Flare project has strict rules for AI usage:

- **All AI usage in any form must be disclosed.** You must state
  the tool you used (e.g. Claude Code, Cursor, Amp) along with
  the extent that the work was AI-assisted.

- **The human-in-the-loop must fully understand all code.** If you
  can't explain what your changes do and how they interact with the
  greater system without the aid of AI tools, do not contribute
  to this project.

- **Issues and discussions can use AI assistance but must have a full
  human-in-the-loop.** This means that any content generated with AI
  must have been reviewed _and edited_ by a human before submission.
  AI is very good at being overly verbose and including noise that
  distracts from the main point. Humans must do their research and
  trim this down.

- **No AI-generated media is allowed (art, images, videos, audio, etc.).**
  Text and code that are **not** part of the game data (i.e. inside the `mods`
  folder) are the only acceptable AI-generated content, per the other rules in
  this policy.

- **Your contributions represent you.** Contributors are assumed to be working
  in good faith. But submitting a flood of low-quality, "slop" contributions
  will result in **single warning**, followed by a **ban** if the behavior is
  not addressed reasonably. "Slop" may have different meanings to different
  people, but I would define it as code that has no respect for the existing
  program's code structure and runtime behavior. For example, code that fails
  to utilize existing functionality and instead opts to plop down its own
  implementation of a function where it pleases. Or, it could be code that
  haphazardly introduces obvious runtime bugs that show that the contributor
  did not test even once.

## There are Humans Here

Please remember that Flare is maintained by humans.

Every discussion, issue, and pull request is read and reviewed by
humans (and sometimes machines, too). It is a boundary point at which
people interact with each other and the work done. It is rude and
disrespectful to approach this boundary with low-effort, unqualified
work, since it puts the burden of validation on the maintainer.

In a perfect world, AI would produce high-quality, accurate work
every time. But today, that reality depends on the driver of the AI.
And today, most drivers of AI are just not good enough. So, until either
the people get better, the AI gets better, or both, we have to have
strict rules to protect maintainers.

