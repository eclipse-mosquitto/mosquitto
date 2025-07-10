class Queue {
  constructor() {
    this.tasks = [];
    this.active = false;
  }

  enqueue(task) {
    this.tasks.push(task);
    this.#dequeue();
  }

  async #dequeue() {
    if (this.active) {
      return;
    }
    this.active = true;
    while (this.tasks.length) {
      const task = this.tasks.shift();
      try {
        await task();
      } catch (err) {
        console.error("Error in queue:", err);
      }
    }
    this.active = false;
  }
}
const queue = new Queue();
