# Contributing to Hologram

## Code of Conduct
We are committed to fostering an open and welcoming environment. Please be respectful and constructive in all interactions.

## How to Contribute

### Reporting Issues
1. Check existing issues to avoid duplicates
2. Use issue templates when available
3. Provide clear reproduction steps
4. Include system information and error messages

### Submitting Pull Requests
1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes
4. Run tests (`make test`)
5. Commit with clear messages
6. Push to your fork
7. Open a pull request against `main`

## Development Guidelines

### Code Style
- **LLVM IR**: Follow LLVM coding standards
- **C Code**: Use K&R style with 4-space indentation
- **Documentation**: Use Markdown with clear headings

### Testing Requirements
- All new code must include tests
- Maintain 100% test pass rate
- Add integration tests for cross-layer features

### Commit Messages
Follow conventional commits:
- `feat:` New features
- `fix:` Bug fixes
- `docs:` Documentation changes
- `test:` Test additions or fixes
- `refactor:` Code restructuring
- `perf:` Performance improvements

### Layer Development
When working on layers:
1. Respect layer boundaries - no cross-layer bypassing
2. Maintain conservation invariants
3. Document all public interfaces
4. Ensure thread safety where applicable

## Review Process
1. Automated tests must pass
2. Code review by maintainer
3. Documentation review if applicable
4. Performance impact assessment for critical paths

## Questions?
- Open a discussion in GitHub Discussions
- Contact maintainers via issues
- Check existing documentation first

Thank you for contributing to Hologram!