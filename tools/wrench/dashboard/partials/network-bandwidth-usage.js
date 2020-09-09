const networkBandwidthHtml = `
    <div class="ui grey tertiary menu small" style="margin-top: -0.5em">
        <div class="header item">
            Chart Options
        </div>
        <div class="ui dropdown item" id="bandwidth-dropdown">
            Bandwidth Usage Unit
            <i class="dropdown icon"></i>
            <div class="menu">
              <a class="item">Bytes</a>
              <a class="item">Kilobytes</a>
              <a class="item">Megabytes</a>
              <a class="item">Gigabytes</a>
              <a class="item">Terabytes</a>
              <a class="item">Petabytes</a>
            </div>
        </div>
    </div>
    <canvas id="network-bandwidth-usage-chart"></canvas>
`
