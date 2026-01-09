### Simple web-based graphical user interface for Mosquitto

To develop UI locally.

1) Install tailwind:
```sh
npm -g install tailwindcss@3
```

2) Go into `src` and run tailwind to generate a CSS file based on tailwind classes used in `index.html`:

```sh
tailwindcss -i ./css/styles.css -o ./tailwind/styles.css
```

3) Run mosquitto http api mock

4) Change mosquitto api endponits in `src/consts.js`

5) Go into `src` and run a simple http server, e.g. `python3 -m http.server 3000`


Dependencies (in `src/lib` directory):

* chartjs 4.3.0 (https://cdnjs.com/libraries/Chart.js/4.3.0)
* chartjs-plugin-zoom 2.2.0 (https://cdnjs.com/libraries/chartjs-plugin-zoom)
* hammer.js 2.0.8 (https://cdnjs.com/libraries/hammer.js)

