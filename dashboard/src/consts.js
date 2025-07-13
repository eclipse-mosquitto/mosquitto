const MAX_POINTS_IN_CHART = 5_000;
const KEEP_DATAPOINTS_FOR_INTERVAL =
  1000 * // 1 sec
  60 * // 1 minute
  60 * // 1 hour
  2; // 2 hours
const CHART_UPDATE_INTERVAL_IN_MILLISECONDS =
  1000 * // 1 sec
  60 * // 1 minute
  1;
const SMOOTHED_CHART_UPDATE_INTERVAL_IN_MILLISECONDS =
  1000 * // 1 sec
  60 * // 1 minute
  5; // 5 minutes
const INTERVAL_5SECS_IN_MILLISECONDS = 1000 * 5;
const CHARTJS_ANIMATION_DURATION_MS = 400;
const SYSTOPIC_ENDPOINT = "/api/v1/systree";
const LISTENERS_ENDPOINT = "/api/v1/listeners";
const CHART_DISPLAY_WINDOW = 16;
