document.addEventListener("DOMContentLoaded", () => {
  new Sidebar();
  new MosquittoDashboard();
});

function checkNormalBannerImage(bannerImage, bannerCard) {
  const imageSrc = "https://mosquitto.org/banner/image"; // no extension on the file - it can svg or png
  const probe = new Image();
  probe.onload = () => {
    bannerImage.src = imageSrc;
  };
  probe.onerror = () => {
    console.warn("Banner-image didn't loaded");
  };
  probe.src = imageSrc;
}

function checkSvgBannerImage(bannerImage, bannerCard, bannerLink, bannerInner) {
  // if a full fledged svg found, display it and remove the default link
  const svgSrc = "https://mosquitto.org/banner/image.svg";
  const svgProbe = new Image();
  svgProbe.onload = () => {
    bannerImage.src = svgSrc;
    // Only if the svg exists (was loaded successfully) make a call to fetch it and then inline it. Requires CORS to be set on svgSrc
    fetch(svgSrc)
      .then((r) => r.text())
      .then((svg) => {
        bannerInner.innerHTML = svg;
        bannerLink.removeAttribute("href");
      })
      .catch((error) =>
        console.warn("SVG banner-image couldn't be fetched:", error),
      );
  };
  svgProbe.onerror = () => {
    console.warn("SVG Banner-image didn't loaded");
  };
  svgProbe.src = svgSrc;
}

document.addEventListener("DOMContentLoaded", function () {
  const toggleButton = document.getElementById("layout-toggle");
  const chartsGrid = document.getElementById("charts-grid");
  const layoutText = document.getElementById("layout-text");
  const bannerImage = document.getElementById("banner-img");
  const bannerCard = document.getElementById("banner-card");
  const bannerInner = document.getElementById("banner-inner");
  const bannerLink = document.getElementById("banner-link");

  checkNormalBannerImage(bannerImage, bannerCard);

  checkSvgBannerImage(bannerImage, bannerCard, bannerLink, bannerInner);

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
