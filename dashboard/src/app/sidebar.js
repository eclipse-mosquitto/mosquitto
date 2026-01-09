class Sidebar {
  constructor() {
    this.menuToggle = document.getElementById("menu-toggle");
    this.menuClose = document.getElementById("menu-close");
    this.slidingMenu = document.getElementById("sliding-menu");
    this.menuOverlay = document.getElementById("menu-overlay");
    this.mainContent = document.getElementById("main-content");
    this.root = document.documentElement;
    this.isOpen = sessionStorage.getItem("isSidebarOpen") === "true";

    // !isMobile() becase we don't want the sidebar to be preloaded as open on mobile - there is no space anyway
    if (!isMobile() && this.isOpen) {
      this.openMenu(); // the initial open of the sidebar is hanlded by the inlined preload script but calling openMenu will properly set hamburger icon to be an arrow icon
    }

    this.bindEvents();
  }

  bindEvents() {
    this.menuToggle.addEventListener("click", () => this.toggleMenu());
    this.menuClose.addEventListener("click", () => this.closeMenu());
    this.menuOverlay.addEventListener("click", () => this.closeMenu());

    document.addEventListener("keydown", (e) => {
      if (e.key === "Escape" && this.isOpen) {
        this.closeMenu();
      }
    });

    window.addEventListener("resize", () => {
      this.syncUi();
    });
  }

  toggleMenu() {
    if (this.isOpen) {
      this.closeMenu();
    } else {
      this.openMenu();
    }
  }

  syncUi() {
    document
      .getElementById("hamburger-icon")
      .classList.toggle("hidden", this.isOpen);
    document
      .getElementById("arrow-icon")
      .classList.toggle("hidden", !this.isOpen);

    const showOverlay = this.isOpen && isMobile();
    document.body.classList.toggle("sidebar-lock-scroll", showOverlay);
  }

  openMenu() {
    this.isOpen = true;
    sessionStorage.setItem("isSidebarOpen", "true");
    this.root.classList.add("sidebar-open");
    this.syncUi();
  }

  closeMenu() {
    this.isOpen = false;
    sessionStorage.setItem("isSidebarOpen", "false");
    this.root.classList.remove("sidebar-open");
    this.syncUi();
  }
}
