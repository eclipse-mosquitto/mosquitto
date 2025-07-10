document.addEventListener("DOMContentLoaded", () => {
  new Sidebar();
  const dashboard = new MosquittoDashboard();
  dashboard.init();
});

document.addEventListener("DOMContentLoaded", function () {
  const toggleButton = document.getElementById("layout-toggle");
  const chartsGrid = document.getElementById("charts-grid");
  const layoutText = document.getElementById("layout-text");

  let isGridView = true;
  let storedSetting = sessionStorage.getItem("isGridView");

  const toggleView = () => {
    if (isGridView) {
      // switch to single column
      chartsGrid.className = "grid grid-cols-1 gap-4";
      layoutText.textContent = "Grid View";
      isGridView = false;
    } else {
      // switch back to grid
      chartsGrid.className = "grid grid-cols-1 lg:grid-cols-2 gap-4";
      layoutText.textContent = "Single Column";
      isGridView = true;
    }
    sessionStorage.setItem("isGridView", JSON.stringify(isGridView));
  };

  if (storedSetting) {
    storedSetting = JSON.parse(storedSetting);
    if (storedSetting === false) {
      // set isGridView from the default value of true to match the "false" coming from the session store by calling the toggle function
      queue.enqueue(toAsyncAndWaitAfter(toggleView));
    }
  }

  toggleButton.addEventListener("click", toggleView);
});
