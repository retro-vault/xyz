# XCC OPTIMIZER PROMPT

You are an expert C compiler developer and Z80 assembly optimization specialist.

I have built a custom C compiler targeting the Z80 processor. The current code generation quality is decent but still produces suboptimal assembly code compared to hand-written Z80 routines. I want to significantly improve the optimization passes of this compiler.

I am providing you with a list of high-quality resources focused on advanced Z80 assembly optimization techniques, peephole optimizations, clever instruction tricks, superoptimization, and common patterns used by the best Z80 programmers.

## Your task
- Carefully study and analyze all the provided resources.
- Extract the most valuable optimization techniques that can be implemented in a compiler (especially peephole optimizations, instruction sequence replacements, register allocation improvements, common subexpression elimination, strength reduction, etc.).
- Create a detailed optimization improvement plan for my Z80 C compiler, prioritized by impact vs implementation difficulty.

For each major proposed optimization, explain:
- What it does
- Why it is effective on Z80
- Rough implementation approach
- Estimated difficulty


## Resources to study

https://github.com/santiontanon/mdlz80optimizer
https://www.cpcwiki.eu/index.php/A_little_optimization_for_Z80_Assembler
https://github.com/oisee/z80-optimizer
https://nanochess.org/article_8_bit_optimization.html
https://wikiti.brandonw.net/index.php?title=Z80_Optimization
https://www.smspower.org/Development/Z80ProgrammingTechniques
https://www.reddit.com/r/Compilers/comments/1q3bpji/a_compiler_for_the_z80/
https://zxpress.ru/article.php?id=8575&lng=eng
https://ftp83plus.net/Tutorials/z80optiA.htm
https://retrocomputing.stackexchange.com/questions/6095/why-do-c-to-z80-compilers-produce-poor-code
https://github.com/Zeda/Z80-Optimized-Routines

## Notes

These techniques range from basic to very advanced (including brute-force superoptimizers). Take your time to understand them thoroughly.

If some techniques are too complex, computationally expensive, or require major architectural changes to the compiler, please flag them clearly and suggest a phased/pipelined approach (quick wins first, then medium, then advanced).

Please begin by confirming you have accessed and studied the resources, then present your analysis and prioritized optimization roadmap.