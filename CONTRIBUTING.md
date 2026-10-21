# Contributing to SprintIDE / 贡献指南

Thank you for your interest in contributing to SprintIDE! / 感谢你关注 SprintIDE 的贡献！

## Code of Conduct / 行为准则

This project is governed by the [Code of Conduct](CODE_OF_CONDUCT.md). By participating, you agree to abide by its terms. / 本项目受 [行为准则](CODE_OF_CONDUCT.md) 约束。参与即表示你同意遵守其条款。

## How to Contribute / 如何贡献

### 1. Report Issues / 提交 Issue

Before opening a new issue, please search existing ones to avoid duplicates. / 提交新 issue 前请先搜索已有 issue，避免重复。

When reporting a bug, include: / 提交 bug 时请提供：

- SprintIDE version / 版本号
- Operating system / 操作系统
- Steps to reproduce / 复现步骤
- Expected vs actual behavior / 期望行为与实际行为
- Screenshots or logs if applicable / 截图或日志（如适用）

### 2. Submit Pull Requests / 提交 PR

**Important:** For significant changes, please open an issue first to discuss your plan before writing code. This saves everyone's time. / **重要：** 重大改动请先开 issue 讨论方案再写代码，避免无效劳动。

Steps: / 步骤：

1. Fork the repository / Fork 本仓库
2. Create a feature branch: `git checkout -b feature/your-feature`
3. Commit your changes: `git commit -m 'feat: add your feature'`
4. Push to the branch: `git push origin feature/your-feature`
5. Open a Pull Request / 提交 Pull Request

### 3. Coding Guidelines / 编码规范

Please follow the coding guidelines in `docs/CODING_GUIDELINES.md`. / 请遵循 `docs/CODING_GUIDELINES.md` 中的编码规范。

Key points: / 要点：

- Use meaningful variable and function names / 使用有意义的变量和函数名
- Write comments for complex logic / 复杂逻辑需添加注释
- Keep functions small and focused / 保持函数短小且职责单一
- Follow the existing code style / 遵循现有代码风格
- Use C++17 or newer as specified in the project / 使用项目中指定的 C++17 或更高版本

### 4. Commit Message Format / 提交信息格式

We follow Conventional Commits: / 我们采用约定式提交：

```
feat: add new feature
fix: resolve a bug
docs: update documentation
style: formatting changes
refactor: code restructuring
test: add or update tests
chore: maintenance tasks
```

### 5. Build & Test / 构建与测试

Make sure your changes build successfully and don't break existing tests. / 确保你的改动能够成功构建，且不破坏现有测试。

See `README.md` for build instructions. / 构建说明请参考 `README.md`。

## Adding Features / 添加功能

When proposing new features, please consider: / 提议新功能时请考虑：

- Does this align with SprintIDE's design philosophy? / 这是否符合 SprintIDE 的设计理念？
- Is this feature within the scope of an IDE, or is it better as a plugin? / 该功能是否在 IDE 范围内，还是更适合作为插件？
- Have you considered backwards compatibility? / 你是否考虑了向后兼容性？

## Translation / 翻译

Translations of documentation and UI strings are welcome. Please open an issue or submit a PR with your translations. / 欢迎提交文档和界面字符串的翻译。请提交 issue 或 PR 提交你的翻译。

## License / 许可证

By contributing to SprintIDE, you agree that your contributions will be licensed under the GNU General Public License v3.0. / 向 SprintIDE 贡献代码即表示你同意你的贡献将在 GNU General Public License v3.0 下授权。

See [LICENSE](LICENSE) for details. / 详情请参阅 [LICENSE](LICENSE)。
