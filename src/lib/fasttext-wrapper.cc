/** *
 * Author: Rafer
 * Date: July 18, 2017 
 * ChangeLog:
 * 1.Ajust to new Fasttext Model.
 * 2.Model only load Once.
 *
 */
#include <iostream>
#include <sstream>
#include <fenv.h>
#include <math.h>
#include <string>

#include <iomanip>
#include <thread>
#include <string>
#include <vector>
#include <algorithm>
#include <map>

#include "fasttext/src/args.h"
#include "fasttext/src/real.h"
#include "fasttext/src/fasttext.h"

#include "fasttext-wrapper.h"


namespace FastTextWrapper {

  std::shared_ptr<fasttext::Args> args_ = std::make_shared<fasttext::Args>();
  std::shared_ptr<fasttext::Dictionary> dict_ = std::make_shared<fasttext::Dictionary>(args_);
  std::shared_ptr<fasttext::Matrix> input_ = std::make_shared<fasttext::Matrix>();
  std::shared_ptr<fasttext::Matrix> output_ = std::make_shared<fasttext::Matrix>();
  std::shared_ptr<fasttext::QMatrix> qinput_ = std::make_shared<fasttext::QMatrix>();
  std::shared_ptr<fasttext::QMatrix> qoutput_ = std::make_shared<fasttext::QMatrix>();

  std::shared_ptr<fasttext::Model> model_ = NULL;

  bool checkModel(std::istream& in) {
    int32_t magic;
    int32_t version;
    in.read((char*)&(magic), sizeof(int32_t));
    if (magic != FASTTEXT_FILEFORMAT_MAGIC_INT32) {
      return false;
    }
    in.read((char*)&(version), sizeof(int32_t));
    if (version != FASTTEXT_VERSION) {
      return false;
    }
    return true;
  }

  FastTextWrapper::FastTextWrapper(): isLoaded(false)
  {
  
  }

  std::map<std::string, std::string> FastTextWrapper::getModelInfo()
  {
    std::map<std::string, std::string> response;
    // dictionary
    response["word_count"] = std::to_string( dict_->nwords() );
    response["label_count"] = std::to_string( dict_->nlabels() );
    response["token_count"] = std::to_string( dict_->ntokens() );

    // arguments
    response["lr"] = std::to_string( args_->lr );
    response["dim"] = std::to_string( args_->dim );
    response["ws"] = std::to_string( args_->ws );
    response["epoch"] = std::to_string( args_->epoch );
    response["minCount"] = std::to_string( args_->minCount );
    response["minCountLabel"] = std::to_string( args_->minCountLabel );
    response["neg"] = std::to_string( args_->neg );
    response["wordNgrams"] = std::to_string( args_->wordNgrams );

    std::string loss_name = "";
    if(args_->loss == fasttext::loss_name::hs)
    {
      loss_name = "hs";
    }
    else if (args_->loss == fasttext::loss_name::ns)
    {
      loss_name = "ns";
    }
    else if (args_->loss == fasttext::loss_name::softmax)
    {
      loss_name = "softmax";
    }

    std::string model_name = "";
    if(args_->model == fasttext::model_name::cbow)
    {
      model_name = "cbow";
    }
    else if (args_->model == fasttext::model_name::sup)
    {
      model_name = "supervised";
    }
    else if (args_->model == fasttext::model_name::sg)
    {
      model_name = "skipgram";
    }

    response["loss"] = loss_name;
    response["model"] = model_name;
    response["bucket"] = std::to_string( args_->bucket );
    response["minn"] = std::to_string( args_->minn );
    response["maxn"] = std::to_string( args_->maxn );
    response["thread"] = std::to_string( args_->thread );
    response["lrUpdateRate"] = std::to_string( args_->lrUpdateRate );
    response["t"] = std::to_string( args_->t );
    response["label"] = args_->label;
    response["verbose"] = std::to_string( args_->verbose );
    response["pretrainedVectors"] = args_->pretrainedVectors;
    return response;
  }

  bool FastTextWrapper::fileExist(const std::string& filename)
  {
    if (FILE *file = fopen(filename.c_str(), "r"))
    {
      fclose(file);
      return true;
    }
    else
    {
      return false;
    }
  }

  std::map<std::string, std::string> FastTextWrapper::train(int argc, char** argv) 
  {
    std::shared_ptr<fasttext::Args> a = std::make_shared<fasttext::Args>();
    a->parseArgs(argc, argv);
    fasttext::FastText fasttext;

    std::cout << "Input >>>>>>>> " << a->input << std::endl;
    std::cout << "Output >>>>>>>> " << a->output << std::endl;

    std::string inputFilename = a->input;
    if ( !fileExist(inputFilename) )
    {
      std::string errorMessage = "Input file is not exist.";
      throw errorMessage;
    }

    fasttext.train(a);
    return loadModel(a->output + ".bin");
  }

  std::vector<std::map<std::string, std::vector<double>>> FastTextWrapper::wordVectors(std::vector<std::string> words)
  {
    std::vector<std::map<std::string, std::vector<double>>> finalResponse(words.size());
    std::cout << "WORD VECTOR" << std::endl;

    fasttext::Vector vec(args_->dim);
    for(uint32_t i = 0; i < words.size(); i++)
    {
      // fasttext get vector
      const std::vector<int32_t>& ngrams = dict_->getNgrams(words[i]);
      vec.zero();
      for (auto it = ngrams.begin(); it != ngrams.end(); ++it) {
        vec.addRow(*input_, *it);
      }

      if (ngrams.size() > 0) {
        vec.mul(1.0 / ngrams.size());
      }

      std::map<std::string, std::vector<double>> response;
      std::vector<double> arr(vec.size());
      for ( int64_t i = 0; i < vec.size(); i++ )
      {
        arr[i] = vec[i];
      }
      
      response[words[i]] = arr;

      finalResponse[i] = response;
    }

    return finalResponse;
  }

  std::vector<std::map<std::string, std::vector<double>>> FastTextWrapper::textVectors(std::vector<std::string> words)
  {
    std::vector<std::map<std::string, std::vector<double>>> finalResponse(words.size());
    std::cout << "TEXT VECTOR" << std::endl;

    std::vector<int32_t> line, labels;
    fasttext::Vector vec(args_->dim);

    for(uint32_t i = 0; i < words.size(); i++)
    {
      std::istringstream input_string(words[i]);
      dict_->getLine(input_string, line, labels, model_->rng);
      // New Version done in getLine()
      //dict_->addNgrams(line,labels, args_->wordNgrams);
      vec.zero();

      for (auto it = line.cbegin(); it != line.cend(); ++it)
      {
        vec.addRow(*input_, *it);
      }

      if (!line.empty())
      {
        vec.mul(1.0 / line.size());
      }

      // std::cout << words[i] << std::endl;
      std::map<std::string, std::vector<double>> response;
      std::vector<double> arr(vec.size());
      for ( int64_t i = 0; i < vec.size(); i++ )
      {
        arr[i] = vec[i];
        // std::cout << vec[i] << std::endl;
      }
      
      response[words[i]] = arr;
      finalResponse[i] = response;

      // std::cout << "==============================" << std::endl;
      // std::cout << "====== debugging mode ========" << std::endl;
      // std::cout << "==============================" << std::endl;

      // for (auto const& iterator : response)
      // {
      //   std::cout << iterator.first << std::endl;
      //   for (uint i = 0; i < iterator.second.size(); ++i)
      //   {
      //     std::cout << iterator.second[i] << std::endl;
      //   }
      // }
    }
    return finalResponse;
  }

  std::vector<std::map<std::string, std::vector<double>>> FastTextWrapper::printVectors(std::string filename, 
    std::vector<std::string> words)
  {
    loadModel(filename);

    if (args_->model == fasttext::model_name::sup) {
      return textVectors(words);
    } else {
      return wordVectors(words);
    }
  }

  std::map<std::string, std::string> FastTextWrapper::loadModel(std::string filename)
  {
    if(isModelLoaded())
    {
      return getModelInfo();
    }

    std::ifstream ifs(filename, std::ifstream::binary);
    if (!ifs.is_open()) {
      std::string errorMessage = "Model file cannot be opened for loading!";
      throw errorMessage;
    }

    if (!checkModel(ifs)) {
      std::cerr << "Model file has wrong file format!" << std::endl;
      exit(EXIT_FAILURE);
    }

    args_->load(ifs);
    dict_->load(ifs);
    bool quant_input;
    ifs.read((char*) &quant_input, sizeof(bool));
    if (quant_input) {
      qinput_->load(ifs);
    } else {
      input_->load(ifs);
    }

    ifs.read((char*) &args_->qout, sizeof(bool));
    if (quant_input && args_->qout) {
      qoutput_->load(ifs);
    } else {
      output_->load(ifs);
    }

    model_ = std::make_shared<fasttext::Model>(input_, output_, args_, 0);

    model_->quant_ = quant_input;
    model_->setQuantizePointer(qinput_, qoutput_, args_->qout);

    if (args_->model == fasttext::model_name::sup) {
      model_->setTargetCounts(dict_->getCounts(fasttext::entry_type::label));
    } else {
      model_->setTargetCounts(dict_->getCounts(fasttext::entry_type::word));
    }

    isLoaded = true;
    ifs.close();

    return getModelInfo();
  }

  std::map<std::string, std::string> FastTextWrapper::test(std::string model, std::string testFile, int32_t k)
  {
    loadModel(model);

    int32_t nexamples = 0, nlabels = 0;
    double precision = 0.0;
    std::vector<int32_t> line, labels;

    std::ifstream ifs(testFile);
    if (!ifs.is_open()) {
      std::string errorMessage = "Test file cannot be opened!";
      throw errorMessage;
    }

    while (ifs.peek() != EOF) {
      dict_->getLine(ifs, line, labels, model_->rng);
      //dict_->addNgrams(line,labels, args_->wordNgrams);
      if (labels.size() > 0 && line.size() > 0) {
        std::vector<std::pair<fasttext::real, int32_t>> modelPredictions;
        model_->predict(line, k, modelPredictions);
        for (auto it = modelPredictions.cbegin(); it != modelPredictions.cend(); it++) {
          if (std::find(labels.begin(), labels.end(), it->second) != labels.end()) {
            precision += 1.0;
          }
        }
        nexamples++;
        nlabels += labels.size();
      }
    }

    ifs.close();
    std::map<std::string, std::string> response;
    double pAtk = precision / (k * nexamples);
    double rAtk = precision / nlabels;
    response["P@" + std::to_string(k) + ""] = std::to_string(pAtk);
    response["R@" + std::to_string(k) + ""] = std::to_string(rAtk);
    response["Number of examples"] = std::to_string(nexamples);

    return response;
  }

  void FastTextWrapper::predict(std::string sentence, int32_t k, std::vector<std::pair<fasttext::real,std::string>>& predictions) const {
    std::vector<int32_t> words, labels;

    std::istringstream input_string(sentence);
    dict_->getLine(input_string, words, labels, model_->rng);
    //dict_->addNgrams(words,labels, args_->wordNgrams);
    if (words.empty()) {
      return;
    }

    fasttext::Vector hidden(args_->dim);
    fasttext::Vector output(dict_->nlabels());
    std::vector<std::pair<fasttext::real,int32_t>> modelPredictions;
    model_->predict(words, k, modelPredictions, hidden, output);
    predictions.clear();
    for (auto it = modelPredictions.cbegin(); it != modelPredictions.cend(); it++) {
      predictions.push_back(std::make_pair(it->first, dict_->getLabel(it->second)));
    }
  }

  std::vector<PredictResult> FastTextWrapper::predict(std::string model, std::vector<std::string> sentences, int32_t k)
  {
    loadModel(model);

    std::vector<std::pair<fasttext::real, std::string>> predictions;

    std::vector<PredictResult> arr;
    for(uint i=0; i < sentences.size(); i++) {
      predict(sentences[i], k, predictions);

      PredictResult response;

      if (predictions.empty()) {
        // std::cout << "n/a" << std::endl;
        response = { sentences[i], "n/a", 0 };
        arr.push_back(response);
        continue;
      }

      for (auto it = predictions.cbegin(); it != predictions.cend(); it++) {
        // std::cout << it->second << ": " << exp(it->first);
        response = { sentences[i], it->second, exp(it->first) };
        arr.push_back(response);
      }

    }
    
    return arr;
  }

}
