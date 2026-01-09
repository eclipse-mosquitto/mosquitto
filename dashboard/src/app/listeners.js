class Listeners {
  constructor() {
    this.abort = new AbortController();
    registerAbortController(this.abort, this);
    this.init();
  }

  async init() {
    try {
      const listeners = await fetchData(LISTENERS_ENDPOINT, {
        signal: this.abort.signal,
        cache: "no-store",
      });
      this.displayListeners(listeners);
    } catch (error) {
      if (
        this.pageHiding ||
        this.abort.signal.aborted ||
        error?.name === "AbortError"
      ) {
        console.log("Fetching listeners aborted");
      } else {
        console.error("Error fetching listeners:", error);
        alert(`Error loading listeners: ${error}`);
      }
    }
  }

  displayListeners(data) {
    const listenersContainer = document.getElementById("listeners-container");
    const brokerAnonymListenerCnt = document.getElementById(
      "broker-anonym-listener-cnt",
    );
    const brokerAllListenerCnt = document.getElementById(
      "broker-all-listener-cnt",
    );
    brokerAnonymListenerCnt.innerHTML = "";
    brokerAllListenerCnt.innerHTML = "";
    listenersContainer.innerHTML = "";

    if (!data || !data.listeners) {
      listenersContainer.innerHTML =
        '<p class="text-gray-500">No listeners available</p>';
      return;
    }

    const listeners = data.listeners;

    brokerAllListenerCnt.textContent = listeners.length;
    const anonymousCount = listeners.filter((l) => l.allow_anonymous).length;

    if (anonymousCount > 0) {
      const warningText = document.createElement("span");
      warningText.textContent = anonymousCount;

      const warningBadge = document.createElement("span");
      warningBadge.className =
        "ml-2 px-2 py-1 text-xs font-medium rounded-full";
      warningBadge.style.backgroundColor = "#fee2e2"; // red-100
      warningBadge.style.color = "#dc2626"; // red-600
      warningBadge.textContent = "UNSAFE";

      brokerAnonymListenerCnt.appendChild(warningText);
      brokerAnonymListenerCnt.appendChild(warningBadge);
    } else {
      brokerAnonymListenerCnt.textContent = "none";
    }

    listeners.forEach((listener, index) => {
      const listenerCard = this.createListenerCard(listener, index);
      listenersContainer.appendChild(listenerCard);
    });
  }

  createCommandSection(listener, type, addMargin = true) {
    const commandSection = document.createElement("div");
    if (addMargin) {
      commandSection.className = "mt-4";
    }

    const commandHeader = document.createElement("div");
    commandHeader.style.display = "flex";
    commandHeader.style.alignItems = "center";
    commandHeader.style.justifyContent = "space-between";
    commandHeader.style.marginBottom = "0.5rem";

    const commandContainer = document.createElement("div");
    commandContainer.style.display = "flex";

    const copyButton = document.createElement("button");
    copyButton.className = "p-2 hover:bg-c-orange transition-colors";
    copyButton.style.cursor = "pointer";
    copyButton.style.border = "0.1px solid #d3d3d3";
    copyButton.title = "Copy";

    // create an svg copy icon
    const copyIcon = document.createElementNS(
      "http://www.w3.org/2000/svg",
      "svg",
    );
    copyIcon.setAttribute("width", "16");
    copyIcon.setAttribute("height", "16");
    copyIcon.setAttribute("viewBox", "0 0 24 24");
    copyIcon.setAttribute("fill", "none");
    copyIcon.setAttribute("stroke", "currentColor");
    copyIcon.setAttribute("stroke-width", "2");
    copyIcon.style.color = "#6b7280"; // gray-500

    copyButton.addEventListener("mouseenter", () => {
      copyIcon.style.stroke = "white";
      checkIcon.style.stroke = "white";
    });

    copyButton.addEventListener("mouseleave", () => {
      copyIcon.style.stroke = "#6b7280"; // back to gray for copy icon and green for the checkmark
      checkIcon.style.stroke = "#10b981";
    });

    const copyPath = document.createElementNS(
      "http://www.w3.org/2000/svg",
      "path",
    );
    copyPath.setAttribute(
      "d",
      "M8 4H6a2 2 0 00-2 2v12a2 2 0 002 2h8a2 2 0 002-2V6a2 2 0 00-2-2h-2m-4-1v1m0 0V2a1 1 0 011-1h2a1 1 0 011 1v1m-4 0h4",
    );
    copyPath.setAttribute("stroke-linecap", "round");
    copyPath.setAttribute("stroke-linejoin", "round");

    copyIcon.appendChild(copyPath);
    copyButton.appendChild(copyIcon);

    // create an svg checkmark icon for the success state after copying
    const checkIcon = document.createElementNS(
      "http://www.w3.org/2000/svg",
      "svg",
    );
    checkIcon.setAttribute("width", "16");
    checkIcon.setAttribute("height", "16");
    checkIcon.setAttribute("viewBox", "0 0 24 24");
    checkIcon.setAttribute("fill", "none");
    checkIcon.setAttribute("stroke", "currentColor");
    checkIcon.setAttribute("stroke-width", "2");
    checkIcon.style.color = "#10b981"; // green-500
    checkIcon.style.display = "none";

    const checkPath = document.createElementNS(
      "http://www.w3.org/2000/svg",
      "path",
    );
    checkPath.setAttribute("d", "M5 13l4 4L19 7");
    checkPath.setAttribute("stroke-linecap", "round");
    checkPath.setAttribute("stroke-linejoin", "round");

    checkIcon.appendChild(checkPath);
    copyButton.appendChild(checkIcon);

    copyButton.addEventListener("click", async () => {
      try {
        const command = this.generateConnectionCommand(listener, type);
        copyToClipboard(command);

        copyIcon.style.display = "none";
        checkIcon.style.display = "block";
        copyButton.title = "Copied!";

        setTimeout(() => {
          copyIcon.style.display = "block";
          checkIcon.style.display = "none";
          copyButton.title = "Copy command";
        }, 2000);
      } catch (err) {
        console.error("Error when copying to clipboard:", err);
      }
    });

    const commandText = document.createElement("pre");
    commandText.className = "bg-gray-100 p-2 text-sm font-mono";
    commandText.style.overflowX = "auto";
    commandText.style.whiteSpace = "pre-wrap";
    commandText.style.width = "100%";
    commandText.style.wordBreak = "break-all";
    commandText.textContent = this.generateConnectionCommand(listener, type);

    commandContainer.appendChild(commandText);
    commandContainer.appendChild(copyButton);
    commandSection.appendChild(commandContainer);

    return commandSection;
  }

  createListenerCard(listener, index) {
    const card = document.createElement("div");
    card.className = "card p-4 border border-gray-200";

    const title = document.createElement("h3");
    title.className = "font-semibold mb-4";
    title.textContent = `Listener ${index + 1}`;

    const details = document.createElement("div");
    details.className = "grid gap-2 text-sm mb-4";

    if (listener.port) {
      details.appendChild(this.createDetailRow("Port", listener.port));
    }
    if (listener.path) {
      details.appendChild(this.createDetailRow("Unix Socket", listener.path));
    }
    if (listener.protocol) {
      details.appendChild(this.createDetailRow("Protocol", listener.protocol));
    }
    details.appendChild(
      this.createDetailRow("TLS", listener.tls ? "Yes" : "No"),
    );
    details.appendChild(
      this.createDetailRow("mTLS", listener.mtls ? "Yes" : "No"),
    );
    details.appendChild(
      this.createDetailRow(
        "Allow Anonymous",
        listener.allow_anonymous ? "Yes" : "No",
      ),
    );

    let commandSectionPub;
    if (listener.protocol !== "httpapi") {
      commandSectionPub = this.createCommandSection(listener, "mosquitto_pub");
    }
    card.appendChild(title);
    card.appendChild(details);
    commandSectionPub && card.appendChild(commandSectionPub);

    return card;
  }

  createDetailRow(label, value) {
    const row = document.createElement("div");
    row.className = "flex items-center";

    const labelSpan = document.createElement("span");
    labelSpan.className = "text-gray-500 mr-2";
    labelSpan.style.width = "120px";
    labelSpan.style.display = "inline-block";
    labelSpan.textContent = label + ":";

    const valueSpan = document.createElement("span");
    valueSpan.className =
      "px-3 py-1 text-xs font-medium rounded-full text-center";
    valueSpan.style.display = "inline-block";
    valueSpan.style.border = "0.1px solid #d3d3d3";
    valueSpan.textContent = value;

    row.appendChild(labelSpan);
    row.appendChild(valueSpan);

    return row;
  }
  generateConnectionCommand(listener, commandType = "mosquitto_pub") {
    if (listener.protocol === "httpapi") {
      return "HTTP API Listener - use REST calls instead of mosquitto_pub/sub";
    }
    let command = commandType;

    if (listener.path) {
      command += ` --unix ${listener.path}`;
    } else {
      command += ` -h <host> -p ${listener.port}`;
    }

    if (listener.mtls) {
      command += " --cert <client-crt.pem> --key <client-key.pem>";
    }

    if (listener.tls) {
      command += " --cafile <ca-crt.pem>";
    }

    if (listener.protocol === "websockets") {
      command += " --ws";
    }

    if (!listener.allow_anonymous) {
      command += " -u <username> -P <password>";
    }

    command += " -t <topic>";

    if (commandType === "mosquitto_pub") {
      command += " -m <message>";
    }

    return command;
  }
}

document.addEventListener("DOMContentLoaded", () => {
  new Sidebar();
  new Listeners();
  new MosquittoDashboard(true);
});
