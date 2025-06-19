async function fetchData(endpoint) {
  if (!endpoint) {
    throw new Error("No endpoint provided to fetch data function");
  }

  let data;
  const res = await fetch(endpoint);
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
