class Sidebar {
  constructor() {
    this.menuToggle = document.getElementById("menu-toggle");
    this.menuClose = document.getElementById("menu-close");
    this.slidingMenu = document.getElementById("sliding-menu");
    this.menuOverlay = document.getElementById("menu-overlay");
    this.mainContent = document.getElementById("main-content");
    this.isOpen = false;

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
      if (this.isOpen) {
        this.updateLayout();
      }
    });
  }

  toggleMenu() {
    if (this.isOpen) {
      this.closeMenu();
    } else {
      this.openMenu();
    }
  }

  openMenu() {
    this.isOpen = true;

    document.getElementById("hamburger-icon").classList.add("hidden");
    document.getElementById("arrow-icon").classList.remove("hidden");

    const isMobile = window.innerWidth < 1024;

    if (isMobile) {
      document.body.style.overflow = "hidden";
      this.menuOverlay.classList.remove("hidden");
      setTimeout(() => {
        this.menuOverlay.classList.remove("opacity-0");
        this.menuOverlay.classList.add("opacity-100");
      }, 10);
    } else {
      this.mainContent.style.marginLeft = "320px";
    }

    this.slidingMenu.classList.remove("-translate-x-full");
    this.slidingMenu.classList.add("translate-x-0");
  }

  closeMenu() {
    this.isOpen = false;

    document.getElementById("hamburger-icon").classList.remove("hidden");
    document.getElementById("arrow-icon").classList.add("hidden");
    document.body.style.overflow = "";
    this.menuOverlay.classList.remove("opacity-100");
    this.menuOverlay.classList.add("opacity-0");
    setTimeout(() => {
      this.menuOverlay.classList.add("hidden");
    }, 300);

    this.mainContent.style.marginLeft = "0";

    this.slidingMenu.classList.remove("translate-x-0");
    this.slidingMenu.classList.add("-translate-x-full");
  }

  updateLayout() {
    if (this.isOpen) {
      this.closeMenu();
      setTimeout(() => this.openMenu(), 50);
    }
  }
}
