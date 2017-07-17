# node-fasttext-wrap

To import: 

### Training

```
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


```

### Test a model

```
FastText.test("model.bin", "data/data_test_c2.test", 1, function (data, error) {
  if(error) {
    console.log(error)
    return;
  }
  console.log(data)
  
});
```


### Predict

```
FastText.predict(
"dbpedia.bin", 1,
['cola lukei , cola lukei is a species of flowering plant in the malvaceae family . it is found only in tanzania . its natural habitat is subtropical or tropical moist lowland forests . it is threatened by habitat loss .'],
function (success, error) {

  if(error) {
    console.log(error)
    return;
  }

  console.log(success[0].label == '__label__11')
  console.log(success)
})
```
