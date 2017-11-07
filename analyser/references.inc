<?php /* Copyright 2014 Karl R. Wilcox

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License. */

class references {
  /**
   * @var DOMDocument $dom
   */
  private $dom;
  /**
   * @var DOMXPath $xpath
   */
  private $xpath;
  
  function __construct($xml) {
      $this->dom = $xml;
      $this->xpath = new DOMXPath($this->dom);
  }

  public function setReferences() {
    // Set IDREF first, to point to appropriate node
    $refList = array (
      array ( '//charge[@keyterm="special/same"]','ancestor::charge[1]|ancestor::ordinary[1]|preceding::charge[1]|preceding::ordinary[1]', true ), // two of the same | another ...
      array ( '//tincture[@origin="ofsame"]','ancestor::tincture[1]|preceding::tincture[1]', true ),// Of the same
      array ( '//tincture[@origin="oflast"]','ancestor::tincture[not(@origin="pending")][1]|preceding::tincture[not(@origin="pending")][1]', true ),// Of the last (in case this is different from of the same?)
//      array ( '//tincture[@origin="pending"]','preceding-sibling::modifier/tincture[1]|following::tincture[1]', false ),// Pending
      array ( '//tincture[@origin="offield"]','ancestor::simple/field/tincture', true ),// Of the field
      array ( '//tincture[@origin="offirst"]','preceding::tincture[colour][last()]', true ),// of the first
      array ( '//tincture[@origin="ofsecond"]','preceding::tincture[colour][last()-1]', true ),// of the second
      array ( '//tincture[@origin="ofthird"]','preceding::tincture[colour][last()-2]', true ),// of the third
      array ( '//tincture[@origin="offourth"]','preceding::tincture[colour][last()-3]', true ),// of the fourth
      );
    $messages = $this->xpath->query('//messages');
    if ( $messages->length == 0 ) {
      $messages = $this->dom->createElement('messages');
      $this->dom->firstChild->appendChild($messages);
    } else
      $messages = $messages->item(0);
    foreach ( $refList as $ref ) {
      /**
       * @var DOMNodeList $nodeList
       */
      $nodeList = $this->xpath->query($ref[0]);
      /**
       * @var DOMElement $sourceNode
       */
      foreach ($nodeList as $sourceNode ) {
        /**
         * @var DOMNodeList $refNodes
         */
        $refNodes = $this->xpath->query($ref[1],$sourceNode);
        if ( $refNodes->length ) {
          /**
           * @var DOMElement $target
           */
          $target = $refNodes->item(0);
          $targetID = $target->getAttribute('ID');
          $sourceNode->setAttribute('IDREF',$targetID);
        } else { // Error reporting
          $sourceTokens = $sourceNode->getAttribute('tokens');
          $messages->appendChild($this->dom->createElement('message',"Cannot find what \"$sourceTokens\" refers to"));
        }
      }
    }
    // Resolve "as many" (Need to set attribute, can't just point to node)
    $nodeList = $this->xpath->query('//charge[@number="asmany"]');
    foreach ($nodeList as $sourceNode ) {
      $refNodes = $this->xpath->query('preceding::charge[@number>"1"]|preceding::ordinary[@number>"1"]',$sourceNode);
      if ( $refNodes->length ) {
        /**
         * @var DOMElement $target
         */
        $target = $refNodes->item(0);
        $sourceNode->setAttribute('number',$target->getAttribute('number'));
      } else {
        $messages->appendChild($this->dom->createElement('message',"Cannot find number for \"as many\" (using \"2\")"));
        $sourceNode->setAttribute('number','2');
      }
    }

    // Resolve tinctures that remain as "pending" (i.e. not given but same as previous - if there was one)
    $nodeList = $this->xpath->query('//tincture[@origin="pending"]');
    foreach ($nodeList as $sourceNode ) {
      $refNodes = $this->xpath->query('preceding-sibling::modifier/tincture[1]|following::tincture[1]',$sourceNode);
      if ( $refNodes->length ) {
        /**
         * @var DOMElement $target
         */
        $target = $refNodes->item(0);
        $targetID = $target->getAttribute('ID');
        $sourceNode->setAttribute('IDREF',$targetID);
      } // else leave as pending, to be found later (in svgutils)
    }

    // Resolve likewise charge adjustments
    $nodeList = $this->xpath->query('//charge/modifier[@keyterm="likewise"]');
    foreach ($nodeList as $sourceNode ) {
      error_log("found likewise");
      $refNodes = $this->xpath->query('preceding::modifier[@type="shown"]',$sourceNode);
      if ( $refNodes->length ) {
      error_log("found previous");
        /**
         * @var DOMElement $target
         */
        foreach ($refNodes as $refNode) {
          $sourceNode->parentNode->appendChild($refNode->cloneNode());
        }
      }  else { // else leave as likewise, it will be ignored
        $messages->appendChild($this->dom->createElement('message',"No previous adjustments for likewise"));
      }
    }

    // Rearrange "charge within ordinary" as "within ordinary charge"
    $nodeList = $this->xpath->query('//modifier[@keyterm="within" and parent::ordinary and child::missing]');
    foreach ($nodeList as $modifierNode) {
      $ordinaryNode = $modifierNode->parentNode;
      $prevCharges = $this->xpath->query('preceding-sibling::charge',$ordinaryNode);
      if ($prevCharges->length) {
        $adjacentCharge = $prevCharges->item($prevCharges->length - 1);
        $modifierNode->replaceChild($adjacentCharge,$modifierNode->firstChild); 
      } else {
            $messages->appendChild($this->dom->createElement('message',"Expected charge for within/inside"));
      }
    }
    return $this->dom;
  }
}
   