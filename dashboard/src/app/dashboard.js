const MAIN_CHART_COLOR = "#fd602e";
const SUPPLEMENTARY_CHART_COLOR = "#6366f1";

class MosquittoDashboard {
  constructor(headless = false) {
    this.abort = new AbortController();
    registerAbortController(this.abort, this);

    this.headlessMode = !!headless;

    !this.headlessMode && window.Chart.register(window.ChartZoom); // chartjs comes from lib/

    this.previousDataFetchFailed = false;
    this.brokerOnline = true;
    this.version = "";
    this.dashboardDataObject = { lastSysTopics: [] };
    const dashboardDataObject = this.composeDashboardObject();
    const [entitiesToUpdate] = this.getElementsToUpdate(
      dashboardDataObject.lastSysTopics,
    );

    // if metrics were present in the session store, update them immediately
    if (!this.headlessMode && entitiesToUpdate) {
      Object.entries(entitiesToUpdate).forEach(([id, value]) => {
        this.updateHtmlElementById(id, value);
      });
    }

    this.dashboardDataObject = dashboardDataObject;

    this.charts = {};
    this.timeoutHandler = null;

    !this.headlessMode && this.initializeCharts();
    !this.headlessMode && this.addToggle();
    this.startDataUpdates();
  }

  getChartDataFromStore(chartId) {
    const chartDataString = sessionStorage.getItem(chartId);
    let chartDataObject = null;
    try {
      if (chartDataString) {
        chartDataObject = JSON.parse(chartDataString);
      }
      return chartDataObject;
    } catch (error) {
      const errorMsg = `Error while creating dashboards: ${error?.message}. Chart data string: "${chartDataString}"`;
      console.error(errorMsg, error);
      alert(errorMsg);
      throw new Error(errorMsg);
    }
  }

  createOptions() {
    return {
      chartDataType: "raw",
    };
  }

  createChartDataObject() {
    // we are already updating dashboardDataObject directly in updateChartInner
    return {
      data: {
        rawData: [],
        smoothedData: [],
      },
      labels: {
        rawLabels: [],
        smoothedLabels: [],
      },
      options: {
        mustUpdate: false,
      },
    };
  }

  composeDashboardObject() {
    const dashboardDataObject = {
      charts: {
        "chart-messages-dropped":
          this.getChartDataFromStore("chart-messages-dropped") ||
          this.createChartDataObject(),
        "chart-messages-sent":
          this.getChartDataFromStore("chart-messages-sent") ||
          this.createChartDataObject(),
        "chart-messages-received":
          this.getChartDataFromStore("chart-messages-received") ||
          this.createChartDataObject(),
        "chart-messages-sent-rate":
          this.getChartDataFromStore("chart-messages-sent-rate") ||
          this.createChartDataObject(),
        "chart-messages-received-rate":
          this.getChartDataFromStore("chart-messages-received-rate") ||
          this.createChartDataObject(),
        "chart-clients-connected":
          this.getChartDataFromStore("chart-clients-connected") ||
          this.createChartDataObject(),
        "chart-clients-disconnected":
          this.getChartDataFromStore("chart-clients-disconnected") ||
          this.createChartDataObject(),
      },
      lastSysTopics: this.getChartDataFromStore("sysTopics") || {},
      lastUpdateDueToIntervalTimestamp:
        this.getChartDataFromStore("updateDueToIntervalTimestamp") || 0, // used to make sure we also always update graphs at certain intervals
      options: this.getChartDataFromStore("options") || this.createOptions(),
    };
    return dashboardDataObject;
  }

  setBrokerVersion() {
    if (this.headlessMode) {
      return;
    }
    this.updateHtmlElementById("broker-version", this.version);
  }

  setBrokerStatus() {
    if (this.headlessMode) {
      return;
    }
    this.removeHtmlElementClass(
      "broker-status",
      this.brokerOnline ? "broker-inactive" : "broker-active",
    );
    this.addHtmlElementClass(
      "broker-status",
      this.brokerOnline ? "broker-active" : "broker-inactive",
    );
    this.updateHtmlElementById(
      "broker-status-text",
      this.brokerOnline ? "Online" : "Offline",
    );
  }

  createLineChart(
    canvasId,
    label,
    color = SUPPLEMENTARY_CHART_COLOR,
    chartDataType,
  ) {
    const labelsType = chartDataType === "raw" ? "rawLabels" : "smoothedLabels";
    const dataType = chartDataType === "raw" ? "rawData" : "smoothedData";
    const ctx = document.getElementById(canvasId).getContext("2d");
    const totalLen =
      this.dashboardDataObject.charts[canvasId].labels[labelsType].length;
    const windowSize = CHART_DISPLAY_WINDOW;
    const startIndex = Math.max(0, totalLen - windowSize);
    const chart = new window.Chart(ctx, {
      type: "line",
      data: {
        labels: this.dashboardDataObject.charts[canvasId].labels[labelsType],
        datasets: [
          {
            label: label,
            data: this.dashboardDataObject.charts[canvasId].data[dataType],
            borderColor: color,
            backgroundColor: color + "20",
            borderWidth: 2,
            fill: true,
            tension: 0.4,
            pointRadius: 3,
            pointHoverRadius: 5,
          },
        ],
      },
      options: {
        responsive: true,
        maintainAspectRatio: false,
        plugins: {
          legend: {
            display: false,
          },
          zoom: {
            zoom: {
              wheel: {
                enabled: true,
              },
              pinch: {
                enabled: true,
              },
              mode: "xy",
            },
            pan: {
              enabled: true,
              mode: "xy",
              rangeMin: startIndex,
              rangeMax: totalLen - 1,
            },
          },
        },
        scales: {
          x: {
            display: true,
            grid: {
              color: "#f3f4f6",
            },
            ticks: {
              font: {
                size: 10,
              },
              maxRotation: 45,
            },
            min: startIndex,
            max: totalLen - 1,
          },
          y: {
            display: true,
            beginAtZero: true,
            grid: {
              color: "#f3f4f6",
            },
            ticks: {
              font: {
                size: 10,
              },
            },
          },
        },
        interaction: {
          intersect: false,
          mode: "index",
        },
      },
    });
    return chart;
  }

  createSentVsReceivedChart(chartDataType) {
    const labelsType = chartDataType === "raw" ? "rawLabels" : "smoothedLabels";
    const dataType = chartDataType === "raw" ? "rawData" : "smoothedData";
    const ctx = document
      .getElementById("chart-message-overview")
      .getContext("2d");
    const totalLen =
      this.dashboardDataObject.charts["chart-messages-sent"].labels[labelsType]
        .length;
    const windowSize = CHART_DISPLAY_WINDOW;
    const startIndex = Math.max(0, totalLen - windowSize);
    const chart = new window.Chart(ctx, {
      type: "line",
      data: {
        labels:
          this.dashboardDataObject.charts["chart-messages-sent"].labels[
            labelsType
          ],
        datasets: [
          {
            label: "Messages Sent",
            data: this.dashboardDataObject.charts["chart-messages-sent"].data[
              dataType
            ],
            borderColor: SUPPLEMENTARY_CHART_COLOR,
            backgroundColor: SUPPLEMENTARY_CHART_COLOR + "20",
            borderWidth: 2,
            fill: false,
            tension: 0.4,
          },
          {
            label: "Messages Received",
            data: this.dashboardDataObject.charts["chart-messages-received"]
              .data[dataType],
            borderColor: MAIN_CHART_COLOR,
            backgroundColor: MAIN_CHART_COLOR + "20",
            borderWidth: 2,
            fill: false,
            tension: 0.4,
          },
        ],
      },
      options: {
        responsive: true,
        maintainAspectRatio: false,
        plugins: {
          legend: {
            display: true,
            position: "top",
          },
          zoom: {
            zoom: {
              wheel: {
                enabled: true,
              },
              pinch: {
                enabled: true,
              },
              mode: "xy",
            },
            pan: {
              enabled: true,
              mode: "xy",
              rangeMin: startIndex,
              rangeMax: totalLen - 1,
            },
          },
        },
        scales: {
          x: {
            display: true,
            grid: {
              color: "#f3f4f6",
            },
            min: startIndex,
            max: totalLen - 1,
          },
          y: {
            display: true,
            beginAtZero: true,
            grid: {
              color: "#f3f4f6",
            },
          },
        },
        interaction: {
          intersect: false,
          mode: "index",
        },
      },
    });

    return chart;
  }

  createRateSentVsReceivedChart(chartDataType) {
    const labelsType = chartDataType === "raw" ? "rawLabels" : "smoothedLabels";
    const dataType = chartDataType === "raw" ? "rawData" : "smoothedData";
    const ctx = document
      .getElementById("chart-message-rate-overview")
      .getContext("2d");
    const totalLen =
      this.dashboardDataObject.charts["chart-messages-sent-rate"].labels[
        labelsType
      ].length;
    const windowSize = CHART_DISPLAY_WINDOW;
    const startIndex = Math.max(0, totalLen - windowSize);
    const chart = new window.Chart(ctx, {
      type: "line",
      data: {
        labels:
          this.dashboardDataObject.charts["chart-messages-sent-rate"].labels[
            labelsType
          ],
        datasets: [
          {
            label: "Messages Sent per Minute",
            data: this.dashboardDataObject.charts["chart-messages-sent-rate"]
              .data[dataType],
            borderColor: SUPPLEMENTARY_CHART_COLOR,
            backgroundColor: SUPPLEMENTARY_CHART_COLOR + "20",
            borderWidth: 2,
            fill: false,
            tension: 0.4,
          },
          {
            label: "Messages Received per Minute",
            data: this.dashboardDataObject.charts[
              "chart-messages-received-rate"
            ].data[dataType],
            borderColor: MAIN_CHART_COLOR,
            backgroundColor: MAIN_CHART_COLOR + "20",
            borderWidth: 2,
            fill: false,
            tension: 0.4,
          },
        ],
      },
      options: {
        responsive: true,
        maintainAspectRatio: false,
        plugins: {
          legend: {
            display: true,
            position: "top",
          },
          zoom: {
            zoom: {
              wheel: {
                enabled: true,
              },
              pinch: {
                enabled: true,
              },
              mode: "xy",
            },
            pan: {
              enabled: true,
              mode: "xy",
              rangeMin: startIndex,
              rangeMax: totalLen - 1,
            },
          },
        },
        scales: {
          x: {
            display: true,
            grid: {
              color: "#f3f4f6",
            },
            min: startIndex,
            max: totalLen - 1,
          },
          y: {
            display: true,
            beginAtZero: true,
            grid: {
              color: "#f3f4f6",
            },
          },
        },
        interaction: {
          intersect: false,
          mode: "index",
        },
      },
    });

    return chart;
  }

  handleChartAction(chartId, action) {
    const chart = this.charts[chartId];
    if (!chart) {
      const errorMsg = "Couldn't find the chart: " + chartId;
      console.error(errorMsg + ". Charts:", Object.keys(this.charts));
      alert(errorMsg);
    }

    switch (action) {
      case "zoom-in":
        chart.zoom(1.2);
        break;
      case "zoom-out":
        chart.zoom(0.8);
        break;
      case "pan-left":
        chart.pan({ x: 100 });
        break;
      case "pan-right":
        chart.pan({ x: -100 });
        break;
      case "reset":
        const newTotalLen = chart.data.labels.length;
        const newStart = Math.max(0, newTotalLen - CHART_DISPLAY_WINDOW);
        chart.options.scales.x.min = newStart;
        chart.options.scales.x.max = newTotalLen - 1;
        chart.update();
        //chart.update("none");
        //Object.values(chart.options.scales).forEach((axisOptions) => {
        //  delete axisOptions.min;
        //  delete axisOptions.max;
        //});
        chart.resetZoom();
        break;
      default:
        const errorMsg = `Unrecognized action "${action}"`;
        console.error(errorMsg);
        alert(errorMsg);
    }
  }

  addToggle() {
    const toggleChartDataTypeButton = document.getElementById(
      "chart-data-type-global-toggle",
    );
    const toggleChartDataTypeText = document.getElementById(
      "chart-data-type-text",
    );

    // set to an opposite state and toggle once to refresh the button captions etc
    if (this.dashboardDataObject.options.chartDataType === "raw") {
      this.dashboardDataObject.options.chartDataType = "smooth";
    } else {
      this.dashboardDataObject.options.chartDataType = "raw";
    }
    const handleChartDataTypeToggle = () => {
      if (this.dashboardDataObject.options.chartDataType === "raw") {
        this.dashboardDataObject.options.chartDataType = "smooth";
        this.destroyCharts();
        toggleChartDataTypeText.textContent = "Show Raw Data";
        this.addHtmlElementClass("smooth-state-svg", "hidden");
        this.removeHtmlElementClass("raw-state-svg", "hidden");
      } else {
        this.dashboardDataObject.options.chartDataType = "raw";
        this.destroyCharts();
        toggleChartDataTypeText.textContent = "Show Smoothed Data";
        this.addHtmlElementClass("raw-state-svg", "hidden");
        this.removeHtmlElementClass("smooth-state-svg", "hidden");
      }
      this.initializeCharts();
      sessionStorage.setItem(
        "options",
        JSON.stringify(this.dashboardDataObject.options),
      );
    };
    toggleChartDataTypeButton.addEventListener("click", () => {
      queue.enqueue(toAsyncAndWaitAfter(handleChartDataTypeToggle));
    });
    queue.enqueue(toAsyncAndWaitAfter(handleChartDataTypeToggle));
  }

  initializeCharts() {
    let id = "";

    id = "chart-messages-dropped";
    this.charts[id] = this.createLineChart(
      id,
      "Dropped Messages",
      MAIN_CHART_COLOR,
      this.dashboardDataObject.options.chartDataType,
    );
    id = "chart-messages-sent";
    this.charts[id] = this.createLineChart(
      id,
      "Messages Sent",
      MAIN_CHART_COLOR,
      this.dashboardDataObject.options.chartDataType,
    );
    id = "chart-messages-received";
    this.charts[id] = this.createLineChart(
      id,
      "Messages Received",
      MAIN_CHART_COLOR,
      this.dashboardDataObject.options.chartDataType,
    );
    id = "chart-messages-sent-rate";
    this.charts[id] = this.createLineChart(
      id,
      "Sent Rate",
      MAIN_CHART_COLOR,
      this.dashboardDataObject.options.chartDataType,
    );
    id = "chart-messages-received-rate";
    this.charts[id] = this.createLineChart(
      id,
      "Received Rate",
      MAIN_CHART_COLOR,
      this.dashboardDataObject.options.chartDataType,
    );
    id = "chart-clients-connected";
    this.charts[id] = this.createLineChart(
      id,
      "Connected Clients",
      MAIN_CHART_COLOR,
      this.dashboardDataObject.options.chartDataType,
    );
    id = "chart-clients-disconnected";
    this.charts[id] = this.createLineChart(
      id,
      "Disconnected Persistent Clients",
      MAIN_CHART_COLOR,
      this.dashboardDataObject.options.chartDataType,
    );
    id = "chart-message-overview";
    this.charts[id] = this.createSentVsReceivedChart(
      this.dashboardDataObject.options.chartDataType,
    );
    id = "chart-message-rate-overview";
    this.charts[id] = this.createRateSentVsReceivedChart(
      this.dashboardDataObject.options.chartDataType,
    );

    document.addEventListener("click", (e) => {
      if (e.target.dataset.action) {
        const chartId = e.target.dataset.chart;
        if (chartId) {
          const action = e.target.dataset.action;
          queue.enqueue(
            toAsyncAndWaitAfter(() => this.handleChartAction(chartId, action)),
          );
        }
      }
    });
  }

  getChartDatasets(chartId) {
    let labels, dataset, dataset2;
    let id1, id2;
    if (chartId === "chart-message-overview") {
      id1 = "chart-messages-sent";
      id2 = "chart-messages-received";
      labels = this.dashboardDataObject.charts[id1].labels;
      dataset = this.dashboardDataObject.charts[id1].data;
      dataset2 = this.dashboardDataObject.charts[id2].data;
      assertExistence(labels, `Labels not found for "${id1}"`);
      assertExistence(labels, `Dataset not found for "${id1}"`);
      assertExistence(labels, `Dataset not found for "${id2}"`);
    } else if (chartId === "chart-message-rate-overview") {
      id1 = "chart-messages-sent-rate";
      id2 = "chart-messages-received-rate";
      labels = this.dashboardDataObject.charts[id1].labels;
      dataset = this.dashboardDataObject.charts[id1].data;
      dataset2 = this.dashboardDataObject.charts[id2].data;
      assertExistence(labels, `Labels not found for "${id1}"`);
      assertExistence(labels, `Dataset not found for "${id1}"`);
      assertExistence(labels, `Dataset not found for "${id2}"`);
    } else {
      labels = this.dashboardDataObject.charts[chartId].labels;
      dataset = this.dashboardDataObject.charts[chartId].data;
      assertExistence(labels, `Labels not found for chartId "${chartId}"`);
      assertExistence(labels, `Dataset not found for chartId "${chartId}"`);
    }

    return [labels, dataset, dataset2];
  }

  getChartPositionalData(chart) {
    const lastX = chart.data.labels.length - 1;
    const secondToLastX = chart.data.labels.length - 2;
    const currentViewFieldEnd = chart.scales.x.max;
    const zoomLevel = chart.getZoomLevel();

    return [zoomLevel, currentViewFieldEnd, lastX, secondToLastX];
  }

  isEndElementVisibleAndDefaultZoom(lastX, currentEnd, zoomLevel) {
    if (currentEnd === lastX && zoomLevel === 1) {
      return true;
    }
    return false;
  }

  slideChart(chart) {
    const newTotalLen = chart.data.labels.length;
    const newStart = Math.max(0, newTotalLen - CHART_DISPLAY_WINDOW);
    chart.options.scales.x.min = newStart;
    chart.options.scales.x.max = newTotalLen - 1;
  }

  destroyCharts() {
    for (const [chartId, _] of Object.entries(this.charts)) {
      let data;
      let data2;
      let labels;
      [labels, data, data2] = this.getChartDatasets(chartId);
      this.charts[chartId].destroy();
    }
  }

  updateLastSysTopics(id, value) {
    this.dashboardDataObject.lastSysTopics[id] = value;
  }

  updateMatchingChart(chartId, sysTopics, chartIdsToUpdate) {
    const createErrorMsg = (matchingChartId, matchingChartSysTopic) =>
      `datapoint doesn't exist in current sysTopic data for the chart "${matchingChartId}" matching the chart "${chartId}". Matching chart sys topic: ${matchingChartSysTopic}. Available sys topics: ${JSON.stringify(
        sysTopics,
      )}`;

    if (chartId === "chart-messages-sent") {
      const matchingChartId = "chart-messages-received";
      const matchingChartSysTopic = "$SYS/broker/messages/received";
      const datapoint = sysTopics[matchingChartSysTopic];
      assertExistence(
        datapoint,
        createErrorMsg(matchingChartId, matchingChartSysTopic),
      );
      chartIdsToUpdate[matchingChartId] = datapoint;
    }
    if (chartId === "chart-messages-received") {
      const matchingChartId = "chart-messages-sent";
      const matchingChartSysTopic = "$SYS/broker/messages/sent";
      const datapoint = sysTopics[matchingChartSysTopic];
      assertExistence(
        datapoint,
        createErrorMsg(matchingChartId, matchingChartSysTopic),
      );
      chartIdsToUpdate[matchingChartId] = datapoint;
    }
    if (chartId === "chart-messages-sent-rate") {
      const matchingChartId = "chart-messages-received-rate";
      const matchingChartSysTopic = "$SYS/broker/load/messages/received/1min";
      const datapoint = sysTopics[matchingChartSysTopic];
      assertExistence(
        datapoint,
        createErrorMsg(matchingChartId, matchingChartSysTopic),
      );
      chartIdsToUpdate[matchingChartId] = datapoint;
    }
    if (chartId === "chart-messages-received-rate") {
      const matchingChartId = "chart-messages-sent-rate";
      const matchingChartSysTopic = "$SYS/broker/load/messages/sent/1min";
      const datapoint = sysTopics[matchingChartSysTopic];
      assertExistence(
        datapoint,
        createErrorMsg(matchingChartId, matchingChartSysTopic),
      );
      chartIdsToUpdate[matchingChartId] = datapoint;
    }
  }

  getElementsToUpdate(sysTopics) {
    // we only update what has actually changed
    let htmlIdsToUpdate = {};
    let chartIdsToUpdate = {};
    let topic = "";

    // sys topics object looks as follows:
    //{
    //  "$SYS/broker/uptime":	99999,
    //  "$SYS/broker/clients/total":	0,
    //  "$SYS/broker/clients/maximum":	1,
    //  "$SYS/broker/clients/disconnected":	0,
    //  "$SYS/broker/clients/connected":	0,
    //  "$SYS/broker/clients/expired":	0,
    //  "$SYS/broker/messages/stored":	2,
    //  "$SYS/broker/store/messages/bytes":	32,
    //  "$SYS/broker/subscriptions/count":	0,
    //  "$SYS/broker/shared_subscriptions/count":	0,
    //  "$SYS/broker/retained messages/count":	2,
    //  "$SYS/broker/heap/current":	796624,
    //  "$SYS/broker/heap/maximum":	796704,
    //  "$SYS/broker/messages/received":	0,
    //  "$SYS/broker/messages/sent":	0,
    //  "$SYS/broker/bytes/received":	0,
    //  "$SYS/broker/bytes/sent":	0,
    //  "$SYS/broker/publish/bytes/received":	0,
    //  "$SYS/broker/publish/bytes/sent":	0,
    //  "$SYS/broker/packet/out/count":	0,
    //  "$SYS/broker/packet/out/bytes":	0,
    //  "$SYS/broker/connections/socket/count":	0,
    //  "$SYS/broker/publish/messages/dropped":	0,
    //  "$SYS/broker/publish/messages/received":	0,
    //  "$SYS/broker/publish/messages/sent":	0
    //}
    topic = "$SYS/broker/uptime";
    if (
      sysTopics[topic] !== undefined &&
      this.dashboardDataObject.lastSysTopics[topic] !== sysTopics[topic]
    ) {
      this.updateLastSysTopics(topic, sysTopics[topic]);
      htmlIdsToUpdate["broker-uptime"] = secondsToIntervalString(
        sysTopics[topic],
      );
    }

    topic = "$SYS/broker/clients/total";
    if (
      sysTopics[topic] !== undefined &&
      this.dashboardDataObject.lastSysTopics[topic] !== sysTopics[topic]
    ) {
      this.updateLastSysTopics(topic, sysTopics[topic]);
      htmlIdsToUpdate["clients-total"] = prettifyNumber(sysTopics[topic]);
      htmlIdsToUpdate["systopic-clients-total"] = sysTopics[topic];
    }

    topic = "$SYS/broker/clients/disconnected";
    if (
      sysTopics[topic] !== undefined &&
      this.dashboardDataObject.lastSysTopics[topic] !== sysTopics[topic]
    ) {
      this.updateLastSysTopics(topic, sysTopics[topic]);
      htmlIdsToUpdate["clients-disconnected"] = prettifyNumber(
        sysTopics[topic],
      );
      htmlIdsToUpdate["systopic-clients-disconnected"] = sysTopics[topic];
      chartIdsToUpdate["chart-clients-disconnected"] = sysTopics[topic];
    }

    topic = "$SYS/broker/clients/connected";
    if (
      sysTopics[topic] !== undefined &&
      this.dashboardDataObject.lastSysTopics[topic] !== sysTopics[topic]
    ) {
      this.updateLastSysTopics(topic, sysTopics[topic]);
      htmlIdsToUpdate["clients-connected"] = prettifyNumber(sysTopics[topic]);
      htmlIdsToUpdate["systopic-clients-connected"] = sysTopics[topic];
      chartIdsToUpdate["chart-clients-connected"] = sysTopics[topic];
    }

    topic = "$SYS/broker/clients/maximum";
    if (
      sysTopics[topic] !== undefined &&
      this.dashboardDataObject.lastSysTopics["clients-maximum"] !==
        sysTopics[topic]
    ) {
      this.updateLastSysTopics("clients-disconnected", sysTopics[topic]);
      htmlIdsToUpdate["clients-maximum"] = sysTopics[topic];
      htmlIdsToUpdate["systopic-clients-max"] = sysTopics[topic];
    }

    topic = "$SYS/broker/clients/expired";
    if (
      sysTopics[topic] !== undefined &&
      this.dashboardDataObject.lastSysTopics[topic] !== sysTopics[topic]
    ) {
      this.updateLastSysTopics(topic, sysTopics[topic]);
      htmlIdsToUpdate["clients-expired"] = prettifyNumber(sysTopics[topic]);
      htmlIdsToUpdate["systopic-clients-expired"] = sysTopics[topic];
    }

    topic = "$SYS/broker/subscriptions/count";
    if (
      sysTopics[topic] !== undefined &&
      this.dashboardDataObject.lastSysTopics[topic] !== sysTopics[topic]
    ) {
      this.updateLastSysTopics(topic, sysTopics[topic]);
      htmlIdsToUpdate["total-subscriptions"] = prettifyNumber(sysTopics[topic]);
      htmlIdsToUpdate["systopic-total-subscriptions"] = sysTopics[topic];
    }

    topic = "$SYS/broker/shared_subscriptions/count";
    if (
      sysTopics[topic] !== undefined &&
      this.dashboardDataObject.lastSysTopics[topic] !== sysTopics[topic]
    ) {
      this.updateLastSysTopics(topic, sysTopics[topic]);
      htmlIdsToUpdate["systopic-total-shared-subscriptions"] = sysTopics[topic];
    }

    topic = "$SYS/broker/heap/current";
    if (
      sysTopics[topic] !== undefined &&
      this.dashboardDataObject.lastSysTopics[topic] !== sysTopics[topic]
    ) {
      this.updateLastSysTopics(topic, sysTopics[topic]);
      htmlIdsToUpdate["systopic-heap-current"] = sysTopics[topic];
    }

    topic = "$SYS/broker/heap/maximum";
    if (
      sysTopics[topic] !== undefined &&
      this.dashboardDataObject.lastSysTopics[topic] !== sysTopics[topic]
    ) {
      this.updateLastSysTopics(topic, sysTopics[topic]);
      htmlIdsToUpdate["systopic-heap-max"] = sysTopics[topic];
    }

    topic = "$SYS/broker/connections/socket/count";
    if (
      sysTopics[topic] !== undefined &&
      this.dashboardDataObject.lastSysTopics[topic] !== sysTopics[topic]
    ) {
      this.updateLastSysTopics(topic, sysTopics[topic]);
      htmlIdsToUpdate["connection-sockets"] = sysTopics[topic];
      htmlIdsToUpdate["systopic-connection-sockets"] = sysTopics[topic];
    }

    topic = "$SYS/broker/load/messages/received/1min";
    if (
      sysTopics[topic] !== undefined &&
      this.dashboardDataObject.lastSysTopics[topic] !== sysTopics[topic]
    ) {
      this.updateLastSysTopics(topic, sysTopics[topic]);
      const chartId = "chart-messages-received-rate";
      chartIdsToUpdate[chartId] = sysTopics[topic];
      this.updateMatchingChart(chartId, sysTopics, chartIdsToUpdate);
      htmlIdsToUpdate["systopic-messages-received-1min"] = sysTopics[topic];
    }

    topic = "$SYS/broker/load/messages/received/10min";
    if (
      sysTopics[topic] !== undefined &&
      this.dashboardDataObject.lastSysTopics[topic] !== sysTopics[topic]
    ) {
      this.updateLastSysTopics(topic, sysTopics[topic]);
      htmlIdsToUpdate["systopic-messages-received-10min"] = sysTopics[topic];
    }

    topic = "$SYS/broker/load/messages/received/15min";
    if (
      sysTopics[topic] !== undefined &&
      this.dashboardDataObject.lastSysTopics[topic] !== sysTopics[topic]
    ) {
      this.updateLastSysTopics(topic, sysTopics[topic]);
      htmlIdsToUpdate["systopic-messages-received-15min"] = sysTopics[topic];
    }

    topic = "$SYS/broker/load/messages/sent/1min";
    if (
      sysTopics[topic] !== undefined &&
      this.dashboardDataObject.lastSysTopics[topic] !== sysTopics[topic]
    ) {
      this.updateLastSysTopics(topic, sysTopics[topic]);
      const chartId = "chart-messages-sent-rate";
      chartIdsToUpdate[chartId] = sysTopics[topic];
      this.updateMatchingChart(chartId, sysTopics, chartIdsToUpdate);
      htmlIdsToUpdate["systopic-messages-sent-1min"] = sysTopics[topic];
    }

    topic = "$SYS/broker/load/messages/sent/10min";
    if (
      sysTopics[topic] !== undefined &&
      this.dashboardDataObject.lastSysTopics[topic] !== sysTopics[topic]
    ) {
      this.updateLastSysTopics(topic, sysTopics[topic]);
      htmlIdsToUpdate["systopic-messages-sent-10min"] = sysTopics[topic];
    }

    topic = "$SYS/broker/load/messages/sent/15min";
    if (
      sysTopics[topic] !== undefined &&
      this.dashboardDataObject.lastSysTopics[topic] !== sysTopics[topic]
    ) {
      this.updateLastSysTopics(topic, sysTopics[topic]);
      htmlIdsToUpdate["systopic-messages-sent-15min"] = sysTopics[topic];
    }

    topic = "$SYS/broker/messages/stored";
    if (
      sysTopics[topic] !== undefined &&
      this.dashboardDataObject.lastSysTopics[topic] !== sysTopics[topic]
    ) {
      this.updateLastSysTopics(topic, sysTopics[topic]);
      htmlIdsToUpdate["messages-stored"] = sysTopics[topic];
      htmlIdsToUpdate["systopic-messages-stored"] = sysTopics[topic];
    }

    topic = "$SYS/broker/retained messages/count";
    if (
      sysTopics[topic] !== undefined &&
      this.dashboardDataObject.lastSysTopics[topic] !== sysTopics[topic]
    ) {
      this.updateLastSysTopics(topic, sysTopics[topic]);
      htmlIdsToUpdate["messages-retained"] = sysTopics[topic];
      htmlIdsToUpdate["systopic-messages-retained"] = sysTopics[topic];
    }

    topic = "$SYS/broker/messages/received";
    if (
      sysTopics[topic] !== undefined &&
      this.dashboardDataObject.lastSysTopics[topic] !== sysTopics[topic]
    ) {
      this.updateLastSysTopics(topic, sysTopics[topic]);
      htmlIdsToUpdate["systopic-messages-received"] = sysTopics[topic];
      const chartId = "chart-messages-received";
      chartIdsToUpdate[chartId] = sysTopics[topic];
      this.updateMatchingChart(chartId, sysTopics, chartIdsToUpdate);
    }

    topic = "$SYS/broker/messages/sent";
    if (
      sysTopics[topic] !== undefined &&
      this.dashboardDataObject.lastSysTopics[topic] !== sysTopics[topic]
    ) {
      this.updateLastSysTopics(topic, sysTopics[topic]);
      htmlIdsToUpdate["systopic-messages-sent"] = sysTopics[topic];
      const chartId = "chart-messages-sent";
      chartIdsToUpdate[chartId] = sysTopics[topic];
      this.updateMatchingChart(chartId, sysTopics, chartIdsToUpdate);
    }

    topic = "$SYS/broker/store/messages/bytes";
    if (
      sysTopics[topic] !== undefined &&
      this.dashboardDataObject.lastSysTopics[topic] !== sysTopics[topic]
    ) {
      this.updateLastSysTopics(topic, sysTopics[topic]);
      htmlIdsToUpdate["systopic-messages-stored-bytes"] = sysTopics[topic];
    }

    topic = "$SYS/broker/bytes/received";
    if (
      sysTopics[topic] !== undefined &&
      this.dashboardDataObject.lastSysTopics[topic] !== sysTopics[topic]
    ) {
      this.updateLastSysTopics(topic, sysTopics[topic]);
      htmlIdsToUpdate["systopic-received-bytes"] = sysTopics[topic];
    }

    topic = "$SYS/broker/bytes/sent";
    if (
      sysTopics[topic] !== undefined &&
      this.dashboardDataObject.lastSysTopics[topic] !== sysTopics[topic]
    ) {
      this.updateLastSysTopics(topic, sysTopics[topic]);
      htmlIdsToUpdate["systopic-sent-bytes"] = sysTopics[topic];
    }

    topic = "$SYS/broker/publish/bytes/received";
    if (
      sysTopics[topic] !== undefined &&
      this.dashboardDataObject.lastSysTopics[topic] !== sysTopics[topic]
    ) {
      this.updateLastSysTopics(topic, sysTopics[topic]);
      htmlIdsToUpdate["systopic-publish-received-bytes"] = sysTopics[topic];
    }

    topic = "$SYS/broker/publish/bytes/sent";
    if (
      sysTopics[topic] !== undefined &&
      this.dashboardDataObject.lastSysTopics[topic] !== sysTopics[topic]
    ) {
      this.updateLastSysTopics(topic, sysTopics[topic]);
      htmlIdsToUpdate["systopic-publish-sent-bytes"] = sysTopics[topic];
    }

    topic = "$SYS/broker/publish/messages/dropped";
    if (
      sysTopics[topic] !== undefined &&
      this.dashboardDataObject.lastSysTopics[topic] !== sysTopics[topic]
    ) {
      this.updateLastSysTopics(topic, sysTopics[topic]);
      htmlIdsToUpdate["messages-dropped"] = sysTopics[topic];
      htmlIdsToUpdate["systopic-messages-dropped"] = sysTopics[topic];
      chartIdsToUpdate["chart-messages-dropped"] = sysTopics[topic];
    }

    topic = "$SYS/broker/publish/messages/received";
    if (
      sysTopics[topic] !== undefined &&
      this.dashboardDataObject.lastSysTopics[topic] !== sysTopics[topic]
    ) {
      this.updateLastSysTopics(topic, sysTopics[topic]);
      htmlIdsToUpdate["messages-published-to-broker"] = sysTopics[topic];
      htmlIdsToUpdate["systopic-messages-published-to-broker"] =
        sysTopics[topic];
    }

    topic = "$SYS/broker/publish/messages/sent";
    if (
      sysTopics[topic] !== undefined &&
      this.dashboardDataObject.lastSysTopics[topic] !== sysTopics[topic]
    ) {
      this.updateLastSysTopics(topic, sysTopics[topic]);
      htmlIdsToUpdate["messages-published-by-broker"] = sysTopics[topic];
      htmlIdsToUpdate["systopic-messages-published-by-broker"] =
        sysTopics[topic];
    }

    topic = "$SYS/broker/packet/out/count";
    if (
      sysTopics[topic] !== undefined &&
      this.dashboardDataObject.lastSysTopics[topic] !== sysTopics[topic]
    ) {
      this.updateLastSysTopics(topic, sysTopics[topic]);
      htmlIdsToUpdate["systopic-out-packets"] = sysTopics[topic];
    }

    topic = "$SYS/broker/packet/out/bytes";
    if (
      sysTopics[topic] !== undefined &&
      this.dashboardDataObject.lastSysTopics[topic] !== sysTopics[topic]
    ) {
      this.updateLastSysTopics(topic, sysTopics[topic]);
      htmlIdsToUpdate["systopic-out-bytes"] = sysTopics[topic];
    }

    if (!Object.keys(htmlIdsToUpdate).length) {
      htmlIdsToUpdate = null;
    }

    if (!Object.keys(chartIdsToUpdate).length) {
      chartIdsToUpdate = null;
    }

    return [htmlIdsToUpdate, chartIdsToUpdate];
  }

  trimChartDataWindow(labels, dataset, timestampNow) {
    let earliestTimestamp = 0;
    while (
      timestampNow - earliestTimestamp >= KEEP_DATAPOINTS_FOR_INTERVAL &&
      labels.length &&
      dataset.length
    ) {
      earliestTimestamp = timeStringToTimestamp(labels[0]);

      labels.shift();
      dataset.shift();
    }
  }

  processChartOverflow(labels, dataset, timestamp) {
    if (!labels.length || !dataset.length) {
      return;
    }
    if (
      labels.length > MAX_POINTS_IN_CHART &&
      dataset.length > MAX_POINTS_IN_CHART
    ) {
      labels.shift();
      dataset.shift();
    }

    if (
      timestamp - timeStringToTimestamp(labels[0]) >=
      KEEP_DATAPOINTS_FOR_INTERVAL
    ) {
      this.trimChartDataWindow(labels, dataset, timestamp);
    }
  }

  datapointsAreSufficientlyDifferent(datapoint1, datapoint2) {
    if (Math.abs(datapoint1 - datapoint2) / datapoint1 > 0.2) {
      return true;
    }
    return false;
  }

  labelsAreFarApart(earlierTimeString, laterTimeString) {
    const earlierTimestamp = timeStringToTimestamp(earlierTimeString);
    const laterTimestamp = timeStringToTimestamp(laterTimeString);

    if (
      laterTimestamp - earlierTimestamp >
      SMOOTHED_CHART_UPDATE_INTERVAL_IN_MILLISECONDS
    ) {
      // also works at the bound between two days because laterTimestamp - earlierTimestamp will in this case be negative, so we return false for the check happening when one timestamp (earlierTimestamp) is coming from the previous day and the current timestamp (laterTimestamp) is for the new day, just after midnight
      return true;
    }
    return false;
  }

  setMustUpdateForMatchingGraph(chartId) {
    const createAssertErrorMsg = (id) =>
      `mustUpdate option not found for chart "${id}". Available options: ${JSON.stringify(
        this.dashboardDataObject.charts[id]?.options,
      )}. Available charts: ${Object.keys(this.dashboardDataObject.charts)}`;
    let oppositeChartId;

    if (chartId === "chart-messages-sent") {
      oppositeChartId = "chart-messages-received";
      assertExistence(
        this.dashboardDataObject.charts[oppositeChartId]?.options?.mustUpdate,
        createAssertErrorMsg(oppositeChartId),
      );
      this.dashboardDataObject.charts[oppositeChartId].options.mustUpdate =
        true;
    } else if (chartId === "chart-messages-received") {
      oppositeChartId = "chart-messages-sent";
      assertExistence(
        this.dashboardDataObject.charts[oppositeChartId]?.options?.mustUpdate,
        createAssertErrorMsg(oppositeChartId),
      );
      this.dashboardDataObject.charts[oppositeChartId].options.mustUpdate =
        true;
    } else if (chartId === "chart-messages-sent-rate") {
      oppositeChartId = "chart-messages-received-rate";
      assertExistence(
        this.dashboardDataObject.charts[oppositeChartId]?.options?.mustUpdate,
        createAssertErrorMsg(oppositeChartId),
      );
      this.dashboardDataObject.charts[oppositeChartId].options.mustUpdate =
        true;
    } else if (chartId === "chart-messages-received-rate") {
      oppositeChartId = "chart-messages-sent-rate";
      assertExistence(
        this.dashboardDataObject.charts[oppositeChartId]?.options?.mustUpdate,
        createAssertErrorMsg(oppositeChartId),
      );
      this.dashboardDataObject.charts[oppositeChartId].options.mustUpdate =
        true;
    }
  }

  addSmoothedDataPoint(
    chartData,
    chartLabels,
    chartOptions,
    datapoint,
    timeString,
    chartId,
  ) {
    const lastElement = chartData.smoothedData.pop();
    const lastLabel = chartLabels.smoothedLabels.pop();

    const smoothedDataLen = chartData.smoothedData.length;
    const smoothedLabelsLen = chartLabels.smoothedLabels.length;
    if (smoothedDataLen != smoothedLabelsLen) {
      const errorMessage = `Smoothed data and label set size deviated: ${smoothedDataLen} vs ${smoothedLabelsLen}. Broken state`;
      throw new Error(errorMessage);
    }

    if (
      lastElement &&
      lastLabel &&
      (chartOptions.mustUpdate ||
        // Compare popped datapoint and the one that is left in the array before it. We check that the differences between datapoints' values reaches a certain threshold or these datapoints have large time interval between insertions
        (smoothedDataLen == 0 && smoothedLabelsLen == 0) ||
        this.datapointsAreSufficientlyDifferent(
          chartData.smoothedData[smoothedDataLen - 1],
          lastElement,
        ) ||
        this.labelsAreFarApart(
          chartLabels.smoothedLabels[smoothedLabelsLen - 1],
          lastLabel,
        ))
    ) {
      !chartOptions.mustUpdate && this.setMustUpdateForMatchingGraph(chartId); // check is needed to avoud a circular update
      chartOptions.mustUpdate = false;
      chartData.smoothedData.push(lastElement);
      chartLabels.smoothedLabels.push(lastLabel);
    }
    // always append the latest datapoint regardless if its value is sufficiently different or not. this is to keep the graph up to date with the latest change and not make it appear stale or simply empty
    chartData.smoothedData.push(datapoint);
    chartLabels.smoothedLabels.push(timeString);
  }

  updateChartInner(id, datapoint, timestamp, dashboardDataObject) {
    const chart = this.charts[id]; // note: in headless mode chart will be undefined as no chart objects are initialized
    const chartData = dashboardDataObject.charts[id].data;
    const chartLabels = dashboardDataObject.charts[id].labels;
    const chartOptions = dashboardDataObject.charts[id].options;

    !this.headlessMode &&
      assertExistence(
        chart,
        `Chart "${id}" not found. Available charts: ${Object.keys(this.charts)}`,
      );
    assertExistence(
      chartData,
      `Data for the chart "${id}" not found. Available charts: ${Object.keys(
        this.dashboardDataObject.charts,
      )}`,
    );
    assertExistence(
      chartLabels,
      `Labels for the chart "${id}" not found. Available charts: ${Object.keys(
        this.dashboardDataObject.charts,
      )}`,
    );
    assertExistence(
      chartOptions,
      `Options for the chart "${id}" not found. Available charts: ${Object.keys(
        this.dashboardDataObject.charts,
      )}`,
    );

    this.processChartOverflow(
      chartLabels.rawLabels,
      chartData.rawData,
      timestamp,
    );

    let zoomLevel, currentEnd, lastX;
    if (!this.headlessMode) {
      [zoomLevel, currentEnd, lastX] = this.getChartPositionalData(chart);
    }

    const timeString = toTimeString(new Date(timestamp));
    chartData.rawData.push(datapoint);
    chartLabels.rawLabels.push(timeString);
    this.addSmoothedDataPoint(
      chartData,
      chartLabels,
      chartOptions,
      datapoint,
      timeString,
      id,
    );

    if (
      !this.headlessMode &&
      this.isEndElementVisibleAndDefaultZoom(lastX, currentEnd, zoomLevel)
    ) {
      this.slideChart(chart);
    }

    !this.headlessMode && chart.update(); // put 'none' for no animation on updates
  }

  getOverviewChartSubchartIds(id) {
    if (id == "chart-message-overview") {
      return ["chart-messages-sent", "chart-messages-received"];
    } else if (id == "chart-message-rate-overview") {
      return ["chart-messages-sent-rate", "chart-messages-received-rate"];
    } else {
      throw new Error(`No such overview chart id: ${id}`);
    }
  }

  updateOverviewChartInner(id, firstSubChartId, secondSubChartId) {
    if (this.headlessMode) {
      // this function only update the chart chart view itself, no data manipulations are done as it simply consumes other line charts. So we have nothing to do in headless mode
      return;
    }
    const chart = this.charts[id];
    const firstChartData =
      this.dashboardDataObject.charts[firstSubChartId].data;
    const firstChartLabels =
      this.dashboardDataObject.charts[firstSubChartId].labels;
    const secondChartData =
      this.dashboardDataObject.charts[secondSubChartId].data;
    const secondChartLabels =
      this.dashboardDataObject.charts[secondSubChartId].labels;

    assertExistence(
      chart,
      `Chart "${id}" not found. Available charts: ${Object.keys(this.charts)}`,
    );
    assertExistence(
      firstChartData,
      `Data for the first sub chart with id "${firstSubChartId}" not found. Available charts: ${Object.keys(
        this.dashboardDataObject.charts,
      )}`,
    );
    assertExistence(
      firstChartLabels,
      `Labels for the first sub chart with id "${firstSubChartId}" not found. Available charts: ${Object.keys(
        this.dashboardDataObject.charts,
      )}`,
    );

    assertExistence(
      secondChartData,
      `Data for the second sub chart with "${secondSubChartId}" not found. Available charts: ${Object.keys(
        this.dashboardDataObject.charts,
      )}`,
    );
    assertExistence(
      secondChartLabels,
      `Labels for the second sub chart with id "${secondSubChartId}" not found. Available charts: ${Object.keys(
        this.dashboardDataObject.charts,
      )}`,
    );

    const [zoomLevel, currentEnd, lastX, secondToLastX] =
      this.getChartPositionalData(chart);

    // compare to both last and second to last x because x may have already moved forward one step since it comes from a separate graph that gets rendered before overview graphs
    if (
      this.isEndElementVisibleAndDefaultZoom(lastX, currentEnd, zoomLevel) ||
      this.isEndElementVisibleAndDefaultZoom(
        secondToLastX,
        currentEnd,
        zoomLevel,
      )
    ) {
      this.slideChart(chart);
    }
    chart.update();
  }

  updateHtmlElementById(elementId, value) {
    const element = document.getElementById(elementId);
    if (element) {
      element.textContent = value;
    }
  }

  removeHtmlElementClass(elementId, className) {
    const element = document.getElementById(elementId);
    if (element) {
      element.classList.remove(className);
    }
  }

  addHtmlElementClass(elementId, className) {
    const element = document.getElementById(elementId);
    if (element) {
      element.classList.add(className);
    }
  }

  updateChart(
    id,
    datapoint,
    timestampMilliseconds,
    dashboardDataObject,
    lastDataPoint,
    isUpdatingAllCharts,
  ) {
    if (datapoint !== undefined || isUpdatingAllCharts) {
      this.updateChartInner(
        id,
        datapoint !== undefined ? datapoint : lastDataPoint,
        timestampMilliseconds,
        dashboardDataObject,
      );
    }
  }

  updateOverviewChart(
    id,
    firstChartDatapoint,
    secondChartDatapoint,
    isUpdatingAllCharts,
    firstSubChartId,
    secondSubChartId,
  ) {
    if (
      firstChartDatapoint !== undefined ||
      secondChartDatapoint !== undefined ||
      isUpdatingAllCharts
    ) {
      this.updateOverviewChartInner(id, firstSubChartId, secondSubChartId);
    }
  }

  getLastChartsDataPoints(dashboardDataObject) {
    const lastChartsDataPoints = {
      "chart-messages-dropped":
        dashboardDataObject.lastSysTopics[
          "$SYS/broker/publish/messages/dropped"
        ],
      "chart-messages-sent":
        dashboardDataObject.lastSysTopics["$SYS/broker/messages/sent"],
      "chart-messages-received":
        dashboardDataObject.lastSysTopics["$SYS/broker/messages/received"],
      "chart-messages-sent-rate":
        dashboardDataObject.lastSysTopics[
          "$SYS/broker/load/messages/sent/1min"
        ],
      "chart-messages-received-rate":
        dashboardDataObject.lastSysTopics[
          "$SYS/broker/load/messages/received/1min"
        ],
      "chart-clients-connected":
        dashboardDataObject.lastSysTopics["$SYS/broker/clients/connected"],
      "chart-clients-disconnected":
        dashboardDataObject.lastSysTopics["$SYS/broker/clients/disconnected"],
    };
    return lastChartsDataPoints;
  }

  updateCharts(
    chartData,
    dashboardDataObject,
    timestampMilliseconds,
    isUpdatingAllCharts,
  ) {
    const lastDataPoints = this.getLastChartsDataPoints(dashboardDataObject);
    let id = "";

    id = "chart-messages-dropped";
    this.updateChart(
      id,
      chartData[id],
      timestampMilliseconds,
      dashboardDataObject,
      lastDataPoints[id],
      isUpdatingAllCharts,
    );

    id = "chart-messages-sent";
    this.updateChart(
      id,
      chartData[id],
      timestampMilliseconds,
      dashboardDataObject,
      lastDataPoints[id],
      isUpdatingAllCharts,
    );

    id = "chart-messages-received";
    this.updateChart(
      id,
      chartData[id],
      timestampMilliseconds,
      dashboardDataObject,
      lastDataPoints[id],
      isUpdatingAllCharts,
    );

    id = "chart-messages-sent-rate";
    this.updateChart(
      id,
      chartData[id],
      timestampMilliseconds,
      dashboardDataObject,
      lastDataPoints[id],
      isUpdatingAllCharts,
    );

    id = "chart-messages-received-rate";
    this.updateChart(
      id,
      chartData[id],
      timestampMilliseconds,
      dashboardDataObject,
      lastDataPoints[id],
      isUpdatingAllCharts,
    );

    id = "chart-clients-connected";
    this.updateChart(
      id,
      chartData[id],
      timestampMilliseconds,
      dashboardDataObject,
      lastDataPoints[id],
      isUpdatingAllCharts,
    );

    id = "chart-clients-disconnected";
    this.updateChart(
      id,
      chartData[id],
      timestampMilliseconds,
      dashboardDataObject,
      lastDataPoints[id],
      isUpdatingAllCharts,
    );

    id = "chart-messages-sent";
    let id2 = "chart-messages-received";
    this.updateOverviewChart(
      "chart-message-overview",
      chartData[id],
      chartData[id2],
      isUpdatingAllCharts,
      id,
      id2,
    );

    id = "chart-messages-sent-rate";
    id2 = "chart-messages-received-rate";
    this.updateOverviewChart(
      "chart-message-rate-overview",
      chartData[id],
      chartData[id2],
      isUpdatingAllCharts,
      id,
      id2,
    );
  }

  mustInsertDatapointDueToInterval(
    lastUpdateDueToIntervalTimestamp,
    nowTimestampMilliseconds,
  ) {
    if (
      lastUpdateDueToIntervalTimestamp === 0 ||
      nowTimestampMilliseconds - lastUpdateDueToIntervalTimestamp >=
        CHART_UPDATE_INTERVAL_IN_MILLISECONDS
    ) {
      return true;
    }
    return false;
  }

  async checkForDataUpdates() {
    const nowTimestampMilliseconds = new Date().getTime();
    let sysTopics = null;
    try {
      sysTopics = await fetchData(SYSTOPIC_ENDPOINT, {
        signal: this.abort.signal,
        cache: "no-store",
      });
      this.version = sysTopics?.["$SYS/broker/version"];

      this.previousDataFetchFailed = false;
      this.brokerOnline = true;
      this.setBrokerVersion();
      this.setBrokerStatus();
    } catch (error) {
      const errorMsg = `Error fetching sys topics: ${error?.message}`;
      if (this.abort.signal.aborted || error?.name === "AbortError") {
        console.log("Fetching systopics aborted");
      } else {
        console.error(errorMsg);
        if (!this.previousDataFetchFailed) {
          alert(errorMsg);
        }
      }
      this.brokerOnline = false;
      this.setBrokerStatus();
      this.previousDataFetchFailed = true;
    }
    if (!sysTopics) {
      return;
    }

    const [metricsToUpdate, chartsToUpdate] =
      this.getElementsToUpdate(sysTopics);
    this.dashboardDataObject.lastSysTopics = sysTopics;

    if (!this.headlessMode && metricsToUpdate) {
      Object.entries(metricsToUpdate).forEach(([id, value]) => {
        this.updateHtmlElementById(id, value);
      });
    }

    let updateAllCharts = false;

    if (
      this.mustInsertDatapointDueToInterval(
        this.dashboardDataObject.lastUpdateDueToIntervalTimestamp,
        nowTimestampMilliseconds,
      )
    ) {
      updateAllCharts = true;
      this.dashboardDataObject.lastUpdateDueToIntervalTimestamp =
        nowTimestampMilliseconds;
    }

    if (chartsToUpdate || updateAllCharts) {
      this.updateCharts(
        chartsToUpdate || {},
        this.dashboardDataObject,
        nowTimestampMilliseconds,
        updateAllCharts,
      );

      let chartsIds;
      if (updateAllCharts) {
        const lastDataPointsOfAllCharts = this.getLastChartsDataPoints(
          this.dashboardDataObject,
        );
        // importantly this gives us ids of all charts
        chartsIds = Object.keys(lastDataPointsOfAllCharts);
      } else if (chartsToUpdate) {
        chartsIds = Object.keys(chartsToUpdate);
      } else {
        chartsIds = []; // nothing to update
      }
      this.updateStore(this.dashboardDataObject, chartsIds);
    }
  }

  updateStore(dashboardDataObject, idsOfChartsToUpdate) {
    try {
      sessionStorage.setItem(
        "options",
        JSON.stringify(dashboardDataObject.options),
      );
      sessionStorage.setItem(
        "sysTopics",
        JSON.stringify(dashboardDataObject.lastSysTopics),
      );
      sessionStorage.setItem(
        "updateDueToIntervalTimestamp",
        JSON.stringify(dashboardDataObject.lastUpdateDueToIntervalTimestamp),
      );
      for (const key of idsOfChartsToUpdate) {
        const chartData = dashboardDataObject.charts[key];
        if (!chartData) {
          throw new Error(
            `dashboardDataObject.charts does not contain key ${key}`,
          );
        }
        sessionStorage.setItem(key, JSON.stringify(chartData));
      }
    } catch (error) {
      const errorMsg = `Error while updating sessionStorage`;
      console.error(errorMsg);
      throw new Error(errorMsg + ": " + error?.message);
    }
  }

  async startDataUpdates() {
    const checkForDataUpdatesWrapper = async () => {
      try {
        await this.checkForDataUpdates();
      } catch (error) {
        const errorMsg = `Error while checking for dashboard data updates ${error?.message}. Reopen the page to try again.`;
        console.error(errorMsg);
        alert(errorMsg);
        throw error;
      }
    };

    try {
      await checkForDataUpdatesWrapper();
    } catch (error) {
      return;
    }
    // we assume that we want to perform data updates every 5 seconds
    const timestampNow = new Date().getTime();
    const nextTimestampDivisibleBy5Seconds =
      INTERVAL_5SECS_IN_MILLISECONDS *
        Math.floor(timestampNow / INTERVAL_5SECS_IN_MILLISECONDS) +
      INTERVAL_5SECS_IN_MILLISECONDS; // integer division and then reconstruct the actual number

    const interval = nextTimestampDivisibleBy5Seconds - timestampNow;

    const doDataUpdate = async () => {
      clearTimeout(this.timeoutHandler);

      let startTs, endTs;
      try {
        startTs = Date.now();
        await checkForDataUpdatesWrapper();
        endTs = Date.now();
      } catch (error) {
        return;
      }
      const executionTimeMs = endTs - startTs;

      this.timeoutHandler = setTimeout(
        // don't want anything to get into a contending state while animation is running, so wait a bit after doDataUpdate returns
        () =>
          queue.enqueue(
            toAsyncAndWaitAfter(
              doDataUpdate,
              CHARTJS_ANIMATION_DURATION_MS + 50,
            ),
          ),
        INTERVAL_5SECS_IN_MILLISECONDS - executionTimeMs > 0
          ? INTERVAL_5SECS_IN_MILLISECONDS - executionTimeMs
          : 0,
      );
    };
    this.timeoutHandler = setTimeout(
      () => queue.enqueue(doDataUpdate),
      interval,
    );
  }
}
