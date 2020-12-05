$ ( document ).ready(function() {
    console.log("Hello, World!")

    var scaleFactor = [1, 1, 1, 1]

    var ctx = document.getElementById('myChart').getContext('2d');
    var myChart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                data: [],
                label: "Specific Gravity",
                borderColor: "#3e95cd",
                fill: false,
                yAxisID: "specGrav"
            },
            {
                data: [],
                label: "ABV",
                borderColor: "#3e05cd",
                fill: false,
                yAxisID: "abv"
            }]
        },
        options: {
            scales: {
                xAxes: [{
                    type: 'time'
                }],
                yAxes: [{
                    id: "specGrav",
                    type: "linear",
                    position: "left",
                    scaleLabel:{
                        display: true,
                        labelString: "Specific Gravity"
                    }
                }, {
                    id: "abv",
                    type: "linear",
                    position: "right",
                    scaleLabel: {
                        display: true,
                        labelString: "% Alcohol by Volume"
                    }
                }]
            },
            title: {
                display: true,
                text: "Specific Gravity and ABV Chart"
            }
        }
    });

    $("#topMarkVal").change(function(){
        topVal  = parseFloat(document.getElementById("topMarkVal").value);
        if (!isNaN(topVal)){
            updateChart()
        }
    });

    $("#markVal").change(function(){
        markVal = parseFloat(document.getElementById("markVal").value);
        if (!isNaN(markVal)){
            updateChart()
        }
    })

    function countToSpecGrav(count, topVal, markVal){
        topVal  = parseFloat(document.getElementById("topMarkVal").value);
        markVal = parseFloat(document.getElementById("markVal").value);
        return topVal + (count * markVal)
    }

    function updateChart(){
        $.get("/getdata", {from_date: "2020-11-18 12:00:00", to_date: "2020-11-19 00:00:00"}, function(data){
            data = $.parseJSON(data);

            dates = [];
            specGravData = [];
            abvData = [];

            origValue = countToSpecGrav(data[0]['count'] * scaleFactor[data[0]['scale']])

            for (var i = 0; i < data.length; i++){
                if (i % 4 == 0){
                    dates.push(new Date(data[i]['date']).toLocaleString());
                }
                currValue = countToSpecGrav(data[i]['count'] * scaleFactor[data[i]['scale']]);
                specGravData.push({t: new Date(data[i]['date']), y: currValue});
                abvData.push({t: new Date(data[i]['date']), y: (origValue - currValue) * 131.25});
            }
            myChart.data.labels = dates;
            myChart.data.datasets[0].data = specGravData;
            myChart.data.datasets[1].data = abvData;
            myChart.update();
        });
    }

    updateChart()
});