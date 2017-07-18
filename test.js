var FastText = require('./index');
var config = { 
  dim: 100,
  input: "data/data_out_c3.txt",
  output: "model"
}

FastText.train("supervised", config, function (success, error) {
 
  if(error) {
    console.log(error)
    return;
  }
  
  console.log(success)
  
})

FastText.test("model.bin", "data/data_test_c2.txt", 1, function (data, error) {
  if(error) {
    console.log(error)
    return;
  }
  console.log(data)
  
});

FastText.predict("model.bin", 1,['我 要 听 周杰伦 的 歌'],function (success, error) {
  if(error) {
    console.log(error)
    return;
  }
  console.log(success)
});