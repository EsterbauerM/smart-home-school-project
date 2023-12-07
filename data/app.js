function toggleCheckbox(element) {
    var xhr = new XMLHttpRequest();
    if(element.checked){ xhr.open("GET", "/update?state=1", true); }
    else { xhr.open("GET", "/update?state=0", true); }
    xhr.send();
  }
  
  setInterval(function ( ) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        var inputChecked;
        var outputStateM;
        if( this.responseText == 1){ 
          inputChecked = true;
          outputStateM = "On";
        }
        else { 
          inputChecked = false;
          outputStateM = "Off";
        }
        document.getElementById("output").checked = inputChecked;
        document.getElementById("outputState").innerHTML = outputStateM;
      }
    };
    xhttp.open("GET", "/state", true);
    xhttp.send();
  }, 1000 ) ;