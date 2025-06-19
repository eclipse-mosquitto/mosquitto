function assertExistence(value, error) {
  if (value === undefined) {
    throw new Error(error);
  }
}

// assertValue(value, expected, error) {}
