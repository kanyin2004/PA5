# PA5
项目简介

本项目为 CS143: Compilers 课程的 Programming Assignment 5 (PA5)。
主要任务是为 COOL（Classroom Object-Oriented Language） 实现 代码生成阶段，将语义正确的 COOL 程序转换为 MIPS 汇编代码。

主要实现内容

实现 COOL 编译器的代码生成模块

构建类表并处理继承关系

生成：

对象布局与方法分发表（dispatch table）

表达式与控制流（if、while、case）

动态 / 静态方法调用

对象创建与 SELF_TYPE

遵循课程提供的 COOL Runtime 调用约定

关键文件
cgen.cc
cgen.h
cool-tree.h
cool-tree.handcode.h

编译与运行
make
./coolc example.cl

说明

基于 CS143 提供的代码框架完成

仅使用课程要求的工具与运行时

作业用途，仅供学习与评测
