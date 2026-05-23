Generate high-quality PNG favicons from the existing SVG

Prerequisites

- Node.js (v16+ recommended) and npm installed locally.

Commands

1. Install dependencies:

```bash
npm install
```

2. Generate PNG favicons (writes to `img/`):

```bash
npm run favicons
```

Files produced

- `img/favicon-16.png`
- `img/favicon-32.png`
- `img/apple-touch-icon.png` (180x180)

Notes

- The script reads `img/favicon.svg` — keep that file as the single source of truth and edit it if you want different branding.
- If you prefer different sizes or formats (ico), you can adjust `scripts/convert-favicons.js` accordingly.