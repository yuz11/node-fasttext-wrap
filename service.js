var FastText = require('./index');
var config = { 
  dim: 100,
  input: "data/data_out_c3.txt",
  output: "model"
}
/*
FastText.train("supervised", config, function (success, error) {
 
  if(error) {
    console.log(error)
    return;
  }
  
  console.log(success)
  
})
*/
FastText.test("model.bin", "data/data_test_c2.test", 1, function (data, error) {
  if(error) {
    console.log(error)
    return;
  }
  console.log(data)
  
});