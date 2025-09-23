# Multi-Book Documentation System

This directory contains a flexible multi-book documentation system built with mdBook, featuring shared themes and easy book creation.

## 📚 Current Books

- **book/** - Hologram: The Physics of Information
- Add your new books here using the template system

## 🚀 Quick Start

### Creating a New Book

```bash
# Create a new book from template
make new BOOK=my-new-book

# Edit the configuration
vim my-new-book/book.toml

# Start development server
make serve BOOK=my-new-book
```

### Working with Existing Books

```bash
# List all available books
make list

# Build a specific book
make build BOOK=book

# Serve a book for development
make serve BOOK=book

# Build all books
make build-all

# Clean build artifacts
make clean BOOK=book
make clean-all
```

## 📂 Directory Structure

```
docs/
├── shared/                 # Shared resources for all books
│   ├── theme/             # Reusable CSS and themes
│   │   └── custom.css     # Shared custom styling
│   ├── config/            # Template configurations
│   │   └── book.template.toml
│   └── templates/         # Template files
│       └── Makefile.template
├── book/                  # Existing Hologram book
│   ├── src/              # Book source files
│   ├── theme/            # Book-specific theme (symlinked)
│   ├── book.toml         # Book configuration
│   └── Makefile          # Book build commands
├── my-new-book/          # Your new book (example)
│   ├── src/              # Book source files
│   ├── theme/            # Symlinked to shared theme
│   ├── book.toml         # Book configuration
│   └── Makefile          # Book build commands
├── Makefile              # Multi-book management
└── README.md             # This file
```

## 🎨 Shared Theming

All books share the same custom CSS theme located in `shared/theme/custom.css`. This ensures:

- Consistent styling across all books
- Single point of maintenance for themes
- Easy updates that apply to all books

The theme includes:
- Fixed code block visibility (light text on dark background)
- Proper print styling for PDF generation
- Responsive design elements
- Custom syntax highlighting

## 📝 Book Configuration

When creating a new book, edit the `book.toml` file to customize:

- **title**: Your book's title
- **authors**: List of authors
- **description**: Book description
- **git-repository-url**: Link to your repository
- **site-url**: Where the book will be hosted

Example:
```toml
[book]
title = "My Technical Guide"
authors = ["Your Name"]
description = "A comprehensive guide to..."
```

## 🛠️ Available Make Commands

### Root Level Commands

| Command | Description | Example |
|---------|-------------|---------|
| `make list` | List all available books | `make list` |
| `make new BOOK=name` | Create a new book from template | `make new BOOK=api-docs` |
| `make build BOOK=name` | Build a specific book | `make build BOOK=book` |
| `make serve BOOK=name` | Start dev server for a book | `make serve BOOK=book` |
| `make clean BOOK=name` | Clean a book's build | `make clean BOOK=book` |
| `make build-all` | Build all books | `make build-all` |
| `make clean-all` | Clean all book builds | `make clean-all` |

### Book Level Commands

Each book has its own Makefile with these commands:

| Command | Description |
|---------|-------------|
| `make build` | Build HTML version |
| `make build-pdf` | Generate PDF (requires wkhtmltopdf) |
| `make serve` | Start dev server with live reload |
| `make clean` | Remove build artifacts |
| `make stats` | Show book statistics |

## 📦 Prerequisites

### Required
- [mdBook](https://rust-lang.github.io/mdBook/) (v0.4.37+)
- Make (for build automation)

### Optional
- wkhtmltopdf (for PDF generation)
- Node.js (for linting tools)

### Installation

```bash
# Install mdBook
cargo install mdbook

# Install wkhtmltopdf (for PDF generation)
# Ubuntu/Debian
sudo apt-get install wkhtmltopdf

# macOS
brew install --cask wkhtmltopdf
```

## 🔧 Customization

### Adding Custom Styling

Edit `shared/theme/custom.css` to modify styling for all books. Changes will automatically apply to all books using the shared theme.

### Book-Specific Styling

If a book needs unique styling:
1. Remove the symlink: `rm book-name/theme/custom.css`
2. Copy the shared CSS: `cp shared/theme/custom.css book-name/theme/`
3. Edit the book-specific CSS file

### Adding Book-Specific Assets

Place assets in the book's directory:
- `book-name/src/images/` for images
- `book-name/src/diagrams/` for diagrams
- Reference them with relative paths in markdown

## 🚢 Deployment

### GitHub Pages

Each book can be deployed to GitHub Pages:

```bash
# Build the book
make build BOOK=my-book

# The output is in my-book/build/
# Deploy this directory to GitHub Pages
```

### Multiple Books on One Domain

You can host multiple books on subpaths:
- `example.com/` - Main documentation
- `example.com/api/` - API documentation
- `example.com/guides/` - User guides

## 📄 PDF Generation

Generate PDFs for any book:

```bash
# Generate PDF for a specific book
cd book-name
make build-pdf

# Output: book-name/build/book-name.pdf
```

PDF generation requires wkhtmltopdf and uses:
- 25mm margins on all sides
- Page numbers in footer
- Table of contents with links
- Print-optimized CSS

## 🤝 Contributing

To add a new book to the system:

1. Create the book: `make new BOOK=your-book-name`
2. Edit `your-book-name/book.toml` with your metadata
3. Build your table of contents in `src/SUMMARY.md`
4. Add your content as markdown files in `src/`
5. Test with `make serve BOOK=your-book-name`
6. Build with `make build BOOK=your-book-name`

## 📚 Example: Creating an API Documentation Book

```bash
# Create the book
make new BOOK=api-docs

# Edit configuration
cat > api-docs/book.toml << 'EOF'
[book]
title = "API Documentation"
authors = ["Engineering Team"]
description = "Complete API reference and guides"
language = "en"
# ... rest of configuration
EOF

# Create structure
cat > api-docs/src/SUMMARY.md << 'EOF'
# Summary

[Introduction](./introduction.md)

---

# API Reference

- [Authentication](./auth.md)
- [Endpoints](./endpoints.md)
  - [Users](./endpoints/users.md)
  - [Projects](./endpoints/projects.md)

# Guides

- [Getting Started](./guides/getting-started.md)
- [Best Practices](./guides/best-practices.md)
EOF

# Start developing
make serve BOOK=api-docs
```

## ❓ Troubleshooting

### Build Fails
```bash
# Clean and rebuild
make clean BOOK=book-name
make build BOOK=book-name
```

### Theme Not Updating
```bash
# Ensure symlink is correct
ls -la book-name/theme/custom.css
# Should show: custom.css -> ../shared/theme/custom.css
```

### PDF Generation Issues
- Ensure wkhtmltopdf is installed
- Check for JavaScript errors in print.html
- Try increasing `--javascript-delay` in Makefile

## 📞 Support

For issues specific to:
- The multi-book system: Create an issue in this repository
- mdBook itself: See [mdBook documentation](https://rust-lang.github.io/mdBook/)
- Individual books: Check the book's repository