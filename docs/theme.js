(() => {
  const STORAGE_KEY = "moos-ivp-docs-theme";
  const ASSET_VERSION = "20260719-1";
  const THEMES = {
    default: null,
    frost: "frost.css",
  };

  const scriptUrl = document.currentScript?.src || document.baseURI;
  const themeBase = new URL("themes/", scriptUrl);
  const params = new URLSearchParams(window.location.search);

  function readStoredTheme() {
    try {
      return localStorage.getItem(STORAGE_KEY);
    } catch {
      return null;
    }
  }

  function writeStoredTheme(theme) {
    try {
      if (theme === "default") {
        localStorage.removeItem(STORAGE_KEY);
      } else {
        localStorage.setItem(STORAGE_KEY, theme);
      }
    } catch {
      // Ignore storage failures; the selector still works for this page load.
    }
  }

  function normalizeTheme(theme) {
    return Object.prototype.hasOwnProperty.call(THEMES, theme) ? theme : "default";
  }

  function selectedTheme() {
    const queryTheme = params.get("theme");
    if (queryTheme && Object.prototype.hasOwnProperty.call(THEMES, queryTheme)) {
      writeStoredTheme(queryTheme);
      return queryTheme;
    }
    return normalizeTheme(readStoredTheme());
  }

  function applyTheme(theme) {
    const normalized = normalizeTheme(theme);
    let link = document.getElementById("theme-css");

    if (!THEMES[normalized]) {
      link?.remove();
      document.documentElement.dataset.theme = "default";
      return normalized;
    }

    if (!link) {
      link = document.createElement("link");
      link.id = "theme-css";
      link.rel = "stylesheet";
      document.head.appendChild(link);
    }

    const href = new URL(THEMES[normalized], themeBase);
    href.searchParams.set("v", ASSET_VERSION);
    link.href = href.href;
    document.documentElement.dataset.theme = normalized;
    return normalized;
  }

  const initialTheme = applyTheme(selectedTheme());

  document.addEventListener("DOMContentLoaded", () => {
    const toggles = document.querySelectorAll("[data-theme-toggle]");
    for (const toggle of toggles) {
      toggle.checked = initialTheme === "frost";
      toggle.addEventListener("change", () => {
        const nextTheme = applyTheme(toggle.checked ? "frost" : "default");
        writeStoredTheme(nextTheme);
        for (const other of toggles) {
          other.checked = nextTheme === "frost";
        }
      });
    }

  });
})();
