function toAsyncAndWaitAfter(task, delay = 0) {
  return () => {
    const promise = new Promise((resolve, reject) => {
      let result;
      try {
        result = task();
      } catch (err) {
        return reject(err);
      }
      if (delay) {
        setTimeout(() => {
          resolve(result);
        }, delay);
      } else {
        resolve(result);
      }
    });
    return promise;
  };
}

async function fetchData(endpoint, opts = {}) {
  if (!endpoint) {
    throw new Error("No endpoint provided to fetch data function");
  }

  let data;
  const res = await fetch(endpoint, {
    ...opts,
    headers: { Accept: "application/json" },
  });
  if (res.ok) {
    data = await res.json();
  } else {
    throw new Error(`Failed to fetch: ${res.status} ${res.statusText}`);
  }

  return data;
}

function toTimeString(date = new Date()) {
  const d = new Date(date);

  const hours = String(d.getHours()).padStart(2, "0");
  const minutes = String(d.getMinutes()).padStart(2, "0");
  const seconds = String(d.getSeconds()).padStart(2, "0");

  return `${hours}:${minutes}:${seconds}`;
}

function timeStringToTimestamp(timeString) {
  const regex = /^(\d{2}):(\d{2}):(\d{2})$/;
  const match = timeString.match(regex);
  if (!match) {
    throw new Error(`Invalid format. Expected HH:mm:ss, got: ${timeString}`);
  }

  const [, hours, minutes, seconds] = match;
  const now = new Date();
  const [year, month, day] = [now.getFullYear(), now.getMonth(), now.getDate()];
  const date = new Date(
    year,
    month,
    day,
    Number(hours),
    Number(minutes),
    Number(seconds),
  );
  return date.getTime();
}

function prettifyNumber(number) {
  if (number > Number.MAX_SAFE_INTEGER) {
    return ">" + String(Number.MAX_SAFE_INTEGER);
  }
  let strNumber = String(number);
  let prettifiedNumber = "";
  if (strNumber.length - 1 > 3) {
    let i = strNumber.length - 3;

    for (; i > 0; i -= 3) {
      prettifiedNumber = "," + strNumber.substring(i, i + 3) + prettifiedNumber;
    }
    prettifiedNumber = strNumber.substring(0, i + 3) + prettifiedNumber;
  } else {
    prettifiedNumber = strNumber;
  }

  return prettifiedNumber;
}

function secondsToIntervalString(number) {
  const minuteInSeconds = 60;
  const hourInSeconds = minuteInSeconds * 60;
  const dayInSeconds = hourInSeconds * 24;
  const yearInSeconds = dayInSeconds * 365;

  if (typeof number !== "number") {
    throw new Error(
      `Invalid datatype for converting into interval string. Expected: number. Got: ${typeof number}`,
    );
  }
  if (number < 0) {
    throw new Error(
      `Invalid value for converting into interval string. Received negative number: ${number}`,
    );
  }

  let intervalString = "";

  const years = Math.floor(number / yearInSeconds);
  number = number % yearInSeconds;
  if (years) {
    intervalString += years === 1 ? "1 year " : `${years} years `;
  }

  const days = Math.floor(number / dayInSeconds);
  number = number % dayInSeconds;
  if (days) {
    intervalString += days === 1 ? "1 day " : `${days} days `;
  }

  const hours = Math.floor(number / hourInSeconds);
  number = number % hourInSeconds;
  if (hours) {
    intervalString += hours === 1 ? "1 hour " : `${hours} hours `;
  }

  const minutes = Math.floor(number / minuteInSeconds);
  number = number % minuteInSeconds;
  if (minutes) {
    intervalString += minutes === 1 ? "1 minute " : `${minutes} minutes `;
  }

  const seconds = number;
  if (seconds) {
    intervalString += seconds === 1 ? "1 second " : `${seconds} seconds `;
  }

  if (!intervalString) {
    return "0 seconds"; // This would be strange if this happened. Maybe better to throw an error
  }

  return intervalString;
}

async function copyToClipboard(textToCopy) {
  if (navigator.clipboard) {
    return await navigator.clipboard.writeText(textToCopy);
  }

  const dummyTextArea = document.createElement("textarea");
  dummyTextArea.value = textToCopy;

  document.body.appendChild(dummyTextArea);
  dummyTextArea.focus({ preventScroll: true });
  dummyTextArea.select();

  try {
    document.execCommand("copy");
  } catch (err) {
    throw new Error("Copy command failed: " + err?.message);
  } finally {
    dummyTextArea.remove();
  }
}

function isMobile() {
  return window.innerWidth < 1024;
}

function registerAbortController(abortController) {
  // in firefox the below doesn't help unfortunately: a general netrowk error is being thrown even before the below callback is executed. A proper implementation would require aborying in-flight requets right before the navigation but it's not worth the effort. Currently you will see an alert for a quick moment when spam-clicking onto the "listern" tab in the sidebar on firefox
  const abortCallback = () => {
    abortController.abort();
  };
  window.addEventListener("pagehide", abortCallback, { once: true });
}
