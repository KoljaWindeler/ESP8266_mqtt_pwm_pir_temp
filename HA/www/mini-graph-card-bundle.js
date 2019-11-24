(function (global, factory) {
  typeof exports === 'object' && typeof module !== 'undefined' ? factory() : typeof define === 'function' && define.amd ? define(factory) : factory();
})(this, function () {
  'use strict';
  /**
   * @license
   * Copyright (c) 2017 The Polymer Project Authors. All rights reserved.
   * This code may only be used under the BSD style license found at
   * http://polymer.github.io/LICENSE.txt
   * The complete set of authors may be found at
   * http://polymer.github.io/AUTHORS.txt
   * The complete set of contributors may be found at
   * http://polymer.github.io/CONTRIBUTORS.txt
   * Code distributed by Google as part of the polymer project is also
   * subject to an additional IP rights grant found at
   * http://polymer.github.io/PATENTS.txt
   */

  const directives = new WeakMap();

  const isDirective = o => {
    return typeof o === 'function' && directives.has(o);
  };
  /**
   * @license
   * Copyright (c) 2017 The Polymer Project Authors. All rights reserved.
   * This code may only be used under the BSD style license found at
   * http://polymer.github.io/LICENSE.txt
   * The complete set of authors may be found at
   * http://polymer.github.io/AUTHORS.txt
   * The complete set of contributors may be found at
   * http://polymer.github.io/CONTRIBUTORS.txt
   * Code distributed by Google as part of the polymer project is also
   * subject to an additional IP rights grant found at
   * http://polymer.github.io/PATENTS.txt
   */

  /**
   * True if the custom elements polyfill is in use.
   */


  const isCEPolyfill = window.customElements !== undefined && window.customElements.polyfillWrapFlushCallback !== undefined;
  /**
   * Reparents nodes, starting from `start` (inclusive) to `end` (exclusive),
   * into another container (could be the same container), before `before`. If
   * `before` is null, it appends the nodes to the container.
   */

  const reparentNodes = (container, start, end = null, before = null) => {
    while (start !== end) {
      const n = start.nextSibling;
      container.insertBefore(start, before);
      start = n;
    }
  };
  /**
   * Removes nodes, starting from `start` (inclusive) to `end` (exclusive), from
   * `container`.
   */


  const removeNodes = (container, start, end = null) => {
    while (start !== end) {
      const n = start.nextSibling;
      container.removeChild(start);
      start = n;
    }
  };
  /**
   * @license
   * Copyright (c) 2018 The Polymer Project Authors. All rights reserved.
   * This code may only be used under the BSD style license found at
   * http://polymer.github.io/LICENSE.txt
   * The complete set of authors may be found at
   * http://polymer.github.io/AUTHORS.txt
   * The complete set of contributors may be found at
   * http://polymer.github.io/CONTRIBUTORS.txt
   * Code distributed by Google as part of the polymer project is also
   * subject to an additional IP rights grant found at
   * http://polymer.github.io/PATENTS.txt
   */

  /**
   * A sentinel value that signals that a value was handled by a directive and
   * should not be written to the DOM.
   */


  const noChange = {};
  /**
   * A sentinel value that signals a NodePart to fully clear its content.
   */

  const nothing = {};
  /**
   * @license
   * Copyright (c) 2017 The Polymer Project Authors. All rights reserved.
   * This code may only be used under the BSD style license found at
   * http://polymer.github.io/LICENSE.txt
   * The complete set of authors may be found at
   * http://polymer.github.io/AUTHORS.txt
   * The complete set of contributors may be found at
   * http://polymer.github.io/CONTRIBUTORS.txt
   * Code distributed by Google as part of the polymer project is also
   * subject to an additional IP rights grant found at
   * http://polymer.github.io/PATENTS.txt
   */

  /**
   * An expression marker with embedded unique key to avoid collision with
   * possible text in templates.
   */

  const marker = `{{lit-${String(Math.random()).slice(2)}}}`;
  /**
   * An expression marker used text-positions, multi-binding attributes, and
   * attributes with markup-like text values.
   */

  const nodeMarker = `<!--${marker}-->`;
  const markerRegex = new RegExp(`${marker}|${nodeMarker}`);
  /**
   * Suffix appended to all bound attribute names.
   */

  const boundAttributeSuffix = '$lit$';
  /**
   * An updateable Template that tracks the location of dynamic parts.
   */

  class Template {
    constructor(result, element) {
      this.parts = [];
      this.element = element;
      const nodesToRemove = [];
      const stack = []; // Edge needs all 4 parameters present; IE11 needs 3rd parameter to be null

      const walker = document.createTreeWalker(element.content, 133
      /* NodeFilter.SHOW_{ELEMENT|COMMENT|TEXT} */
      , null, false); // Keeps track of the last index associated with a part. We try to delete
      // unnecessary nodes, but we never want to associate two different parts
      // to the same index. They must have a constant node between.

      let lastPartIndex = 0;
      let index = -1;
      let partIndex = 0;
      const {
        strings,
        values: {
          length
        }
      } = result;

      while (partIndex < length) {
        const node = walker.nextNode();

        if (node === null) {
          // We've exhausted the content inside a nested template element.
          // Because we still have parts (the outer for-loop), we know:
          // - There is a template in the stack
          // - The walker will find a nextNode outside the template
          walker.currentNode = stack.pop();
          continue;
        }

        index++;

        if (node.nodeType === 1
        /* Node.ELEMENT_NODE */
        ) {
            if (node.hasAttributes()) {
              const attributes = node.attributes;
              const {
                length
              } = attributes; // Per
              // https://developer.mozilla.org/en-US/docs/Web/API/NamedNodeMap,
              // attributes are not guaranteed to be returned in document order.
              // In particular, Edge/IE can return them out of order, so we cannot
              // assume a correspondence between part index and attribute index.

              let count = 0;

              for (let i = 0; i < length; i++) {
                if (endsWith(attributes[i].name, boundAttributeSuffix)) {
                  count++;
                }
              }

              while (count-- > 0) {
                // Get the template literal section leading up to the first
                // expression in this attribute
                const stringForPart = strings[partIndex]; // Find the attribute name

                const name = lastAttributeNameRegex.exec(stringForPart)[2]; // Find the corresponding attribute
                // All bound attributes have had a suffix added in
                // TemplateResult#getHTML to opt out of special attribute
                // handling. To look up the attribute value we also need to add
                // the suffix.

                const attributeLookupName = name.toLowerCase() + boundAttributeSuffix;
                const attributeValue = node.getAttribute(attributeLookupName);
                node.removeAttribute(attributeLookupName);
                const statics = attributeValue.split(markerRegex);
                this.parts.push({
                  type: 'attribute',
                  index,
                  name,
                  strings: statics
                });
                partIndex += statics.length - 1;
              }
            }

            if (node.tagName === 'TEMPLATE') {
              stack.push(node);
              walker.currentNode = node.content;
            }
          } else if (node.nodeType === 3
        /* Node.TEXT_NODE */
        ) {
            const data = node.data;

            if (data.indexOf(marker) >= 0) {
              const parent = node.parentNode;
              const strings = data.split(markerRegex);
              const lastIndex = strings.length - 1; // Generate a new text node for each literal section
              // These nodes are also used as the markers for node parts

              for (let i = 0; i < lastIndex; i++) {
                let insert;
                let s = strings[i];

                if (s === '') {
                  insert = createMarker();
                } else {
                  const match = lastAttributeNameRegex.exec(s);

                  if (match !== null && endsWith(match[2], boundAttributeSuffix)) {
                    s = s.slice(0, match.index) + match[1] + match[2].slice(0, -boundAttributeSuffix.length) + match[3];
                  }

                  insert = document.createTextNode(s);
                }

                parent.insertBefore(insert, node);
                this.parts.push({
                  type: 'node',
                  index: ++index
                });
              } // If there's no text, we must insert a comment to mark our place.
              // Else, we can trust it will stick around after cloning.


              if (strings[lastIndex] === '') {
                parent.insertBefore(createMarker(), node);
                nodesToRemove.push(node);
              } else {
                node.data = strings[lastIndex];
              } // We have a part for each match found


              partIndex += lastIndex;
            }
          } else if (node.nodeType === 8
        /* Node.COMMENT_NODE */
        ) {
            if (node.data === marker) {
              const parent = node.parentNode; // Add a new marker node to be the startNode of the Part if any of
              // the following are true:
              //  * We don't have a previousSibling
              //  * The previousSibling is already the start of a previous part

              if (node.previousSibling === null || index === lastPartIndex) {
                index++;
                parent.insertBefore(createMarker(), node);
              }

              lastPartIndex = index;
              this.parts.push({
                type: 'node',
                index
              }); // If we don't have a nextSibling, keep this node so we have an end.
              // Else, we can remove it to save future costs.

              if (node.nextSibling === null) {
                node.data = '';
              } else {
                nodesToRemove.push(node);
                index--;
              }

              partIndex++;
            } else {
              let i = -1;

              while ((i = node.data.indexOf(marker, i + 1)) !== -1) {
                // Comment node has a binding marker inside, make an inactive part
                // The binding won't work, but subsequent bindings will
                // TODO (justinfagnani): consider whether it's even worth it to
                // make bindings in comments work
                this.parts.push({
                  type: 'node',
                  index: -1
                });
                partIndex++;
              }
            }
          }
      } // Remove text binding nodes after the walk to not disturb the TreeWalker


      for (const n of nodesToRemove) {
        n.parentNode.removeChild(n);
      }
    }

  }

  const endsWith = (str, suffix) => {
    const index = str.length - suffix.length;
    return index >= 0 && str.slice(index) === suffix;
  };

  const isTemplatePartActive = part => part.index !== -1; // Allows `document.createComment('')` to be renamed for a
  // small manual size-savings.


  const createMarker = () => document.createComment('');
  /**
   * This regex extracts the attribute name preceding an attribute-position
   * expression. It does this by matching the syntax allowed for attributes
   * against the string literal directly preceding the expression, assuming that
   * the expression is in an attribute-value position.
   *
   * See attributes in the HTML spec:
   * https://www.w3.org/TR/html5/syntax.html#elements-attributes
   *
   * " \x09\x0a\x0c\x0d" are HTML space characters:
   * https://www.w3.org/TR/html5/infrastructure.html#space-characters
   *
   * "\0-\x1F\x7F-\x9F" are Unicode control characters, which includes every
   * space character except " ".
   *
   * So an attribute is:
   *  * The name: any character except a control character, space character, ('),
   *    ("), ">", "=", or "/"
   *  * Followed by zero or more space characters
   *  * Followed by "="
   *  * Followed by zero or more space characters
   *  * Followed by:
   *    * Any character except space, ('), ("), "<", ">", "=", (`), or
   *    * (") then any non-("), or
   *    * (') then any non-(')
   */


  const lastAttributeNameRegex = /([ \x09\x0a\x0c\x0d])([^\0-\x1F\x7F-\x9F "'>=/]+)([ \x09\x0a\x0c\x0d]*=[ \x09\x0a\x0c\x0d]*(?:[^ \x09\x0a\x0c\x0d"'`<>=]*|"[^"]*|'[^']*))$/;
  /**
   * @license
   * Copyright (c) 2017 The Polymer Project Authors. All rights reserved.
   * This code may only be used under the BSD style license found at
   * http://polymer.github.io/LICENSE.txt
   * The complete set of authors may be found at
   * http://polymer.github.io/AUTHORS.txt
   * The complete set of contributors may be found at
   * http://polymer.github.io/CONTRIBUTORS.txt
   * Code distributed by Google as part of the polymer project is also
   * subject to an additional IP rights grant found at
   * http://polymer.github.io/PATENTS.txt
   */

  /**
   * An instance of a `Template` that can be attached to the DOM and updated
   * with new values.
   */

  class TemplateInstance {
    constructor(template, processor, options) {
      this.__parts = [];
      this.template = template;
      this.processor = processor;
      this.options = options;
    }

    update(values) {
      let i = 0;

      for (const part of this.__parts) {
        if (part !== undefined) {
          part.setValue(values[i]);
        }

        i++;
      }

      for (const part of this.__parts) {
        if (part !== undefined) {
          part.commit();
        }
      }
    }

    _clone() {
      // There are a number of steps in the lifecycle of a template instance's
      // DOM fragment:
      //  1. Clone - create the instance fragment
      //  2. Adopt - adopt into the main document
      //  3. Process - find part markers and create parts
      //  4. Upgrade - upgrade custom elements
      //  5. Update - set node, attribute, property, etc., values
      //  6. Connect - connect to the document. Optional and outside of this
      //     method.
      //
      // We have a few constraints on the ordering of these steps:
      //  * We need to upgrade before updating, so that property values will pass
      //    through any property setters.
      //  * We would like to process before upgrading so that we're sure that the
      //    cloned fragment is inert and not disturbed by self-modifying DOM.
      //  * We want custom elements to upgrade even in disconnected fragments.
      //
      // Given these constraints, with full custom elements support we would
      // prefer the order: Clone, Process, Adopt, Upgrade, Update, Connect
      //
      // But Safari dooes not implement CustomElementRegistry#upgrade, so we
      // can not implement that order and still have upgrade-before-update and
      // upgrade disconnected fragments. So we instead sacrifice the
      // process-before-upgrade constraint, since in Custom Elements v1 elements
      // must not modify their light DOM in the constructor. We still have issues
      // when co-existing with CEv0 elements like Polymer 1, and with polyfills
      // that don't strictly adhere to the no-modification rule because shadow
      // DOM, which may be created in the constructor, is emulated by being placed
      // in the light DOM.
      //
      // The resulting order is on native is: Clone, Adopt, Upgrade, Process,
      // Update, Connect. document.importNode() performs Clone, Adopt, and Upgrade
      // in one step.
      //
      // The Custom Elements v1 polyfill supports upgrade(), so the order when
      // polyfilled is the more ideal: Clone, Process, Adopt, Upgrade, Update,
      // Connect.
      const fragment = isCEPolyfill ? this.template.element.content.cloneNode(true) : document.importNode(this.template.element.content, true);
      const stack = [];
      const parts = this.template.parts; // Edge needs all 4 parameters present; IE11 needs 3rd parameter to be null

      const walker = document.createTreeWalker(fragment, 133
      /* NodeFilter.SHOW_{ELEMENT|COMMENT|TEXT} */
      , null, false);
      let partIndex = 0;
      let nodeIndex = 0;
      let part;
      let node = walker.nextNode(); // Loop through all the nodes and parts of a template

      while (partIndex < parts.length) {
        part = parts[partIndex];

        if (!isTemplatePartActive(part)) {
          this.__parts.push(undefined);

          partIndex++;
          continue;
        } // Progress the tree walker until we find our next part's node.
        // Note that multiple parts may share the same node (attribute parts
        // on a single element), so this loop may not run at all.


        while (nodeIndex < part.index) {
          nodeIndex++;

          if (node.nodeName === 'TEMPLATE') {
            stack.push(node);
            walker.currentNode = node.content;
          }

          if ((node = walker.nextNode()) === null) {
            // We've exhausted the content inside a nested template element.
            // Because we still have parts (the outer for-loop), we know:
            // - There is a template in the stack
            // - The walker will find a nextNode outside the template
            walker.currentNode = stack.pop();
            node = walker.nextNode();
          }
        } // We've arrived at our part's node.


        if (part.type === 'node') {
          const part = this.processor.handleTextExpression(this.options);
          part.insertAfterNode(node.previousSibling);

          this.__parts.push(part);
        } else {
          this.__parts.push(...this.processor.handleAttributeExpressions(node, part.name, part.strings, this.options));
        }

        partIndex++;
      }

      if (isCEPolyfill) {
        document.adoptNode(fragment);
        customElements.upgrade(fragment);
      }

      return fragment;
    }

  }
  /**
   * @license
   * Copyright (c) 2017 The Polymer Project Authors. All rights reserved.
   * This code may only be used under the BSD style license found at
   * http://polymer.github.io/LICENSE.txt
   * The complete set of authors may be found at
   * http://polymer.github.io/AUTHORS.txt
   * The complete set of contributors may be found at
   * http://polymer.github.io/CONTRIBUTORS.txt
   * Code distributed by Google as part of the polymer project is also
   * subject to an additional IP rights grant found at
   * http://polymer.github.io/PATENTS.txt
   */


  const commentMarker = ` ${marker} `;
  /**
   * The return type of `html`, which holds a Template and the values from
   * interpolated expressions.
   */

  class TemplateResult {
    constructor(strings, values, type, processor) {
      this.strings = strings;
      this.values = values;
      this.type = type;
      this.processor = processor;
    }
    /**
     * Returns a string of HTML used to create a `<template>` element.
     */


    getHTML() {
      const l = this.strings.length - 1;
      let html = '';
      let isCommentBinding = false;

      for (let i = 0; i < l; i++) {
        const s = this.strings[i]; // For each binding we want to determine the kind of marker to insert
        // into the template source before it's parsed by the browser's HTML
        // parser. The marker type is based on whether the expression is in an
        // attribute, text, or comment poisition.
        //   * For node-position bindings we insert a comment with the marker
        //     sentinel as its text content, like <!--{{lit-guid}}-->.
        //   * For attribute bindings we insert just the marker sentinel for the
        //     first binding, so that we support unquoted attribute bindings.
        //     Subsequent bindings can use a comment marker because multi-binding
        //     attributes must be quoted.
        //   * For comment bindings we insert just the marker sentinel so we don't
        //     close the comment.
        //
        // The following code scans the template source, but is *not* an HTML
        // parser. We don't need to track the tree structure of the HTML, only
        // whether a binding is inside a comment, and if not, if it appears to be
        // the first binding in an attribute.

        const commentOpen = s.lastIndexOf('<!--'); // We're in comment position if we have a comment open with no following
        // comment close. Because <-- can appear in an attribute value there can
        // be false positives.

        isCommentBinding = (commentOpen > -1 || isCommentBinding) && s.indexOf('-->', commentOpen + 1) === -1; // Check to see if we have an attribute-like sequence preceeding the
        // expression. This can match "name=value" like structures in text,
        // comments, and attribute values, so there can be false-positives.

        const attributeMatch = lastAttributeNameRegex.exec(s);

        if (attributeMatch === null) {
          // We're only in this branch if we don't have a attribute-like
          // preceeding sequence. For comments, this guards against unusual
          // attribute values like <div foo="<!--${'bar'}">. Cases like
          // <!-- foo=${'bar'}--> are handled correctly in the attribute branch
          // below.
          html += s + (isCommentBinding ? commentMarker : nodeMarker);
        } else {
          // For attributes we use just a marker sentinel, and also append a
          // $lit$ suffix to the name to opt-out of attribute-specific parsing
          // that IE and Edge do for style and certain SVG attributes.
          html += s.substr(0, attributeMatch.index) + attributeMatch[1] + attributeMatch[2] + boundAttributeSuffix + attributeMatch[3] + marker;
        }
      }

      html += this.strings[l];
      return html;
    }

    getTemplateElement() {
      const template = document.createElement('template');
      template.innerHTML = this.getHTML();
      return template;
    }

  }
  /**
   * A TemplateResult for SVG fragments.
   *
   * This class wraps HTML in an `<svg>` tag in order to parse its contents in the
   * SVG namespace, then modifies the template to remove the `<svg>` tag so that
   * clones only container the original fragment.
   */


  class SVGTemplateResult extends TemplateResult {
    getHTML() {
      return `<svg>${super.getHTML()}</svg>`;
    }

    getTemplateElement() {
      const template = super.getTemplateElement();
      const content = template.content;
      const svgElement = content.firstChild;
      content.removeChild(svgElement);
      reparentNodes(content, svgElement.firstChild);
      return template;
    }

  }
  /**
   * @license
   * Copyright (c) 2017 The Polymer Project Authors. All rights reserved.
   * This code may only be used under the BSD style license found at
   * http://polymer.github.io/LICENSE.txt
   * The complete set of authors may be found at
   * http://polymer.github.io/AUTHORS.txt
   * The complete set of contributors may be found at
   * http://polymer.github.io/CONTRIBUTORS.txt
   * Code distributed by Google as part of the polymer project is also
   * subject to an additional IP rights grant found at
   * http://polymer.github.io/PATENTS.txt
   */


  const isPrimitive = value => {
    return value === null || !(typeof value === 'object' || typeof value === 'function');
  };

  const isIterable = value => {
    return Array.isArray(value) || // tslint:disable-next-line:no-any
    !!(value && value[Symbol.iterator]);
  };
  /**
   * Writes attribute values to the DOM for a group of AttributeParts bound to a
   * single attibute. The value is only set once even if there are multiple parts
   * for an attribute.
   */


  class AttributeCommitter {
    constructor(element, name, strings) {
      this.dirty = true;
      this.element = element;
      this.name = name;
      this.strings = strings;
      this.parts = [];

      for (let i = 0; i < strings.length - 1; i++) {
        this.parts[i] = this._createPart();
      }
    }
    /**
     * Creates a single part. Override this to create a differnt type of part.
     */


    _createPart() {
      return new AttributePart(this);
    }

    _getValue() {
      const strings = this.strings;
      const l = strings.length - 1;
      let text = '';

      for (let i = 0; i < l; i++) {
        text += strings[i];
        const part = this.parts[i];

        if (part !== undefined) {
          const v = part.value;

          if (isPrimitive(v) || !isIterable(v)) {
            text += typeof v === 'string' ? v : String(v);
          } else {
            for (const t of v) {
              text += typeof t === 'string' ? t : String(t);
            }
          }
        }
      }

      text += strings[l];
      return text;
    }

    commit() {
      if (this.dirty) {
        this.dirty = false;
        this.element.setAttribute(this.name, this._getValue());
      }
    }

  }
  /**
   * A Part that controls all or part of an attribute value.
   */


  class AttributePart {
    constructor(committer) {
      this.value = undefined;
      this.committer = committer;
    }

    setValue(value) {
      if (value !== noChange && (!isPrimitive(value) || value !== this.value)) {
        this.value = value; // If the value is a not a directive, dirty the committer so that it'll
        // call setAttribute. If the value is a directive, it'll dirty the
        // committer if it calls setValue().

        if (!isDirective(value)) {
          this.committer.dirty = true;
        }
      }
    }

    commit() {
      while (isDirective(this.value)) {
        const directive$$1 = this.value;
        this.value = noChange;
        directive$$1(this);
      }

      if (this.value === noChange) {
        return;
      }

      this.committer.commit();
    }

  }
  /**
   * A Part that controls a location within a Node tree. Like a Range, NodePart
   * has start and end locations and can set and update the Nodes between those
   * locations.
   *
   * NodeParts support several value types: primitives, Nodes, TemplateResults,
   * as well as arrays and iterables of those types.
   */


  class NodePart {
    constructor(options) {
      this.value = undefined;
      this.__pendingValue = undefined;
      this.options = options;
    }
    /**
     * Appends this part into a container.
     *
     * This part must be empty, as its contents are not automatically moved.
     */


    appendInto(container) {
      this.startNode = container.appendChild(createMarker());
      this.endNode = container.appendChild(createMarker());
    }
    /**
     * Inserts this part after the `ref` node (between `ref` and `ref`'s next
     * sibling). Both `ref` and its next sibling must be static, unchanging nodes
     * such as those that appear in a literal section of a template.
     *
     * This part must be empty, as its contents are not automatically moved.
     */


    insertAfterNode(ref) {
      this.startNode = ref;
      this.endNode = ref.nextSibling;
    }
    /**
     * Appends this part into a parent part.
     *
     * This part must be empty, as its contents are not automatically moved.
     */


    appendIntoPart(part) {
      part.__insert(this.startNode = createMarker());

      part.__insert(this.endNode = createMarker());
    }
    /**
     * Inserts this part after the `ref` part.
     *
     * This part must be empty, as its contents are not automatically moved.
     */


    insertAfterPart(ref) {
      ref.__insert(this.startNode = createMarker());

      this.endNode = ref.endNode;
      ref.endNode = this.startNode;
    }

    setValue(value) {
      this.__pendingValue = value;
    }

    commit() {
      while (isDirective(this.__pendingValue)) {
        const directive$$1 = this.__pendingValue;
        this.__pendingValue = noChange;
        directive$$1(this);
      }

      const value = this.__pendingValue;

      if (value === noChange) {
        return;
      }

      if (isPrimitive(value)) {
        if (value !== this.value) {
          this.__commitText(value);
        }
      } else if (value instanceof TemplateResult) {
        this.__commitTemplateResult(value);
      } else if (value instanceof Node) {
        this.__commitNode(value);
      } else if (isIterable(value)) {
        this.__commitIterable(value);
      } else if (value === nothing) {
        this.value = nothing;
        this.clear();
      } else {
        // Fallback, will render the string representation
        this.__commitText(value);
      }
    }

    __insert(node) {
      this.endNode.parentNode.insertBefore(node, this.endNode);
    }

    __commitNode(value) {
      if (this.value === value) {
        return;
      }

      this.clear();

      this.__insert(value);

      this.value = value;
    }

    __commitText(value) {
      const node = this.startNode.nextSibling;
      value = value == null ? '' : value; // If `value` isn't already a string, we explicitly convert it here in case
      // it can't be implicitly converted - i.e. it's a symbol.

      const valueAsString = typeof value === 'string' ? value : String(value);

      if (node === this.endNode.previousSibling && node.nodeType === 3
      /* Node.TEXT_NODE */
      ) {
          // If we only have a single text node between the markers, we can just
          // set its value, rather than replacing it.
          // TODO(justinfagnani): Can we just check if this.value is primitive?
          node.data = valueAsString;
        } else {
        this.__commitNode(document.createTextNode(valueAsString));
      }

      this.value = value;
    }

    __commitTemplateResult(value) {
      const template = this.options.templateFactory(value);

      if (this.value instanceof TemplateInstance && this.value.template === template) {
        this.value.update(value.values);
      } else {
        // Make sure we propagate the template processor from the TemplateResult
        // so that we use its syntax extension, etc. The template factory comes
        // from the render function options so that it can control template
        // caching and preprocessing.
        const instance = new TemplateInstance(template, value.processor, this.options);

        const fragment = instance._clone();

        instance.update(value.values);

        this.__commitNode(fragment);

        this.value = instance;
      }
    }

    __commitIterable(value) {
      // For an Iterable, we create a new InstancePart per item, then set its
      // value to the item. This is a little bit of overhead for every item in
      // an Iterable, but it lets us recurse easily and efficiently update Arrays
      // of TemplateResults that will be commonly returned from expressions like:
      // array.map((i) => html`${i}`), by reusing existing TemplateInstances.
      // If _value is an array, then the previous render was of an
      // iterable and _value will contain the NodeParts from the previous
      // render. If _value is not an array, clear this part and make a new
      // array for NodeParts.
      if (!Array.isArray(this.value)) {
        this.value = [];
        this.clear();
      } // Lets us keep track of how many items we stamped so we can clear leftover
      // items from a previous render


      const itemParts = this.value;
      let partIndex = 0;
      let itemPart;

      for (const item of value) {
        // Try to reuse an existing part
        itemPart = itemParts[partIndex]; // If no existing part, create a new one

        if (itemPart === undefined) {
          itemPart = new NodePart(this.options);
          itemParts.push(itemPart);

          if (partIndex === 0) {
            itemPart.appendIntoPart(this);
          } else {
            itemPart.insertAfterPart(itemParts[partIndex - 1]);
          }
        }

        itemPart.setValue(item);
        itemPart.commit();
        partIndex++;
      }

      if (partIndex < itemParts.length) {
        // Truncate the parts array so _value reflects the current state
        itemParts.length = partIndex;
        this.clear(itemPart && itemPart.endNode);
      }
    }

    clear(startNode = this.startNode) {
      removeNodes(this.startNode.parentNode, startNode.nextSibling, this.endNode);
    }

  }
  /**
   * Implements a boolean attribute, roughly as defined in the HTML
   * specification.
   *
   * If the value is truthy, then the attribute is present with a value of
   * ''. If the value is falsey, the attribute is removed.
   */


  class BooleanAttributePart {
    constructor(element, name, strings) {
      this.value = undefined;
      this.__pendingValue = undefined;

      if (strings.length !== 2 || strings[0] !== '' || strings[1] !== '') {
        throw new Error('Boolean attributes can only contain a single expression');
      }

      this.element = element;
      this.name = name;
      this.strings = strings;
    }

    setValue(value) {
      this.__pendingValue = value;
    }

    commit() {
      while (isDirective(this.__pendingValue)) {
        const directive$$1 = this.__pendingValue;
        this.__pendingValue = noChange;
        directive$$1(this);
      }

      if (this.__pendingValue === noChange) {
        return;
      }

      const value = !!this.__pendingValue;

      if (this.value !== value) {
        if (value) {
          this.element.setAttribute(this.name, '');
        } else {
          this.element.removeAttribute(this.name);
        }

        this.value = value;
      }

      this.__pendingValue = noChange;
    }

  }
  /**
   * Sets attribute values for PropertyParts, so that the value is only set once
   * even if there are multiple parts for a property.
   *
   * If an expression controls the whole property value, then the value is simply
   * assigned to the property under control. If there are string literals or
   * multiple expressions, then the strings are expressions are interpolated into
   * a string first.
   */


  class PropertyCommitter extends AttributeCommitter {
    constructor(element, name, strings) {
      super(element, name, strings);
      this.single = strings.length === 2 && strings[0] === '' && strings[1] === '';
    }

    _createPart() {
      return new PropertyPart(this);
    }

    _getValue() {
      if (this.single) {
        return this.parts[0].value;
      }

      return super._getValue();
    }

    commit() {
      if (this.dirty) {
        this.dirty = false; // tslint:disable-next-line:no-any

        this.element[this.name] = this._getValue();
      }
    }

  }

  class PropertyPart extends AttributePart {} // Detect event listener options support. If the `capture` property is read
  // from the options object, then options are supported. If not, then the thrid
  // argument to add/removeEventListener is interpreted as the boolean capture
  // value so we should only pass the `capture` property.


  let eventOptionsSupported = false;

  try {
    const options = {
      get capture() {
        eventOptionsSupported = true;
        return false;
      }

    }; // tslint:disable-next-line:no-any

    window.addEventListener('test', options, options); // tslint:disable-next-line:no-any

    window.removeEventListener('test', options, options);
  } catch (_e) {}

  class EventPart {
    constructor(element, eventName, eventContext) {
      this.value = undefined;
      this.__pendingValue = undefined;
      this.element = element;
      this.eventName = eventName;
      this.eventContext = eventContext;

      this.__boundHandleEvent = e => this.handleEvent(e);
    }

    setValue(value) {
      this.__pendingValue = value;
    }

    commit() {
      while (isDirective(this.__pendingValue)) {
        const directive$$1 = this.__pendingValue;
        this.__pendingValue = noChange;
        directive$$1(this);
      }

      if (this.__pendingValue === noChange) {
        return;
      }

      const newListener = this.__pendingValue;
      const oldListener = this.value;
      const shouldRemoveListener = newListener == null || oldListener != null && (newListener.capture !== oldListener.capture || newListener.once !== oldListener.once || newListener.passive !== oldListener.passive);
      const shouldAddListener = newListener != null && (oldListener == null || shouldRemoveListener);

      if (shouldRemoveListener) {
        this.element.removeEventListener(this.eventName, this.__boundHandleEvent, this.__options);
      }

      if (shouldAddListener) {
        this.__options = getOptions(newListener);
        this.element.addEventListener(this.eventName, this.__boundHandleEvent, this.__options);
      }

      this.value = newListener;
      this.__pendingValue = noChange;
    }

    handleEvent(event) {
      if (typeof this.value === 'function') {
        this.value.call(this.eventContext || this.element, event);
      } else {
        this.value.handleEvent(event);
      }
    }

  } // We copy options because of the inconsistent behavior of browsers when reading
  // the third argument of add/removeEventListener. IE11 doesn't support options
  // at all. Chrome 41 only reads `capture` if the argument is an object.


  const getOptions = o => o && (eventOptionsSupported ? {
    capture: o.capture,
    passive: o.passive,
    once: o.once
  } : o.capture);
  /**
   * @license
   * Copyright (c) 2017 The Polymer Project Authors. All rights reserved.
   * This code may only be used under the BSD style license found at
   * http://polymer.github.io/LICENSE.txt
   * The complete set of authors may be found at
   * http://polymer.github.io/AUTHORS.txt
   * The complete set of contributors may be found at
   * http://polymer.github.io/CONTRIBUTORS.txt
   * Code distributed by Google as part of the polymer project is also
   * subject to an additional IP rights grant found at
   * http://polymer.github.io/PATENTS.txt
   */

  /**
   * Creates Parts when a template is instantiated.
   */


  class DefaultTemplateProcessor {
    /**
     * Create parts for an attribute-position binding, given the event, attribute
     * name, and string literals.
     *
     * @param element The element containing the binding
     * @param name  The attribute name
     * @param strings The string literals. There are always at least two strings,
     *   event for fully-controlled bindings with a single expression.
     */
    handleAttributeExpressions(element, name, strings, options) {
      const prefix = name[0];

      if (prefix === '.') {
        const committer = new PropertyCommitter(element, name.slice(1), strings);
        return committer.parts;
      }

      if (prefix === '@') {
        return [new EventPart(element, name.slice(1), options.eventContext)];
      }

      if (prefix === '?') {
        return [new BooleanAttributePart(element, name.slice(1), strings)];
      }

      const committer = new AttributeCommitter(element, name, strings);
      return committer.parts;
    }
    /**
     * Create parts for a text-position binding.
     * @param templateFactory
     */


    handleTextExpression(options) {
      return new NodePart(options);
    }

  }

  const defaultTemplateProcessor = new DefaultTemplateProcessor();
  /**
   * @license
   * Copyright (c) 2017 The Polymer Project Authors. All rights reserved.
   * This code may only be used under the BSD style license found at
   * http://polymer.github.io/LICENSE.txt
   * The complete set of authors may be found at
   * http://polymer.github.io/AUTHORS.txt
   * The complete set of contributors may be found at
   * http://polymer.github.io/CONTRIBUTORS.txt
   * Code distributed by Google as part of the polymer project is also
   * subject to an additional IP rights grant found at
   * http://polymer.github.io/PATENTS.txt
   */

  /**
   * The default TemplateFactory which caches Templates keyed on
   * result.type and result.strings.
   */

  function templateFactory(result) {
    let templateCache = templateCaches.get(result.type);

    if (templateCache === undefined) {
      templateCache = {
        stringsArray: new WeakMap(),
        keyString: new Map()
      };
      templateCaches.set(result.type, templateCache);
    }

    let template = templateCache.stringsArray.get(result.strings);

    if (template !== undefined) {
      return template;
    } // If the TemplateStringsArray is new, generate a key from the strings
    // This key is shared between all templates with identical content


    const key = result.strings.join(marker); // Check if we already have a Template for this key

    template = templateCache.keyString.get(key);

    if (template === undefined) {
      // If we have not seen this key before, create a new Template
      template = new Template(result, result.getTemplateElement()); // Cache the Template for this key

      templateCache.keyString.set(key, template);
    } // Cache all future queries for this TemplateStringsArray


    templateCache.stringsArray.set(result.strings, template);
    return template;
  }

  const templateCaches = new Map();
  /**
   * @license
   * Copyright (c) 2017 The Polymer Project Authors. All rights reserved.
   * This code may only be used under the BSD style license found at
   * http://polymer.github.io/LICENSE.txt
   * The complete set of authors may be found at
   * http://polymer.github.io/AUTHORS.txt
   * The complete set of contributors may be found at
   * http://polymer.github.io/CONTRIBUTORS.txt
   * Code distributed by Google as part of the polymer project is also
   * subject to an additional IP rights grant found at
   * http://polymer.github.io/PATENTS.txt
   */

  const parts = new WeakMap();
  /**
   * Renders a template result or other value to a container.
   *
   * To update a container with new values, reevaluate the template literal and
   * call `render` with the new result.
   *
   * @param result Any value renderable by NodePart - typically a TemplateResult
   *     created by evaluating a template tag like `html` or `svg`.
   * @param container A DOM parent to render to. The entire contents are either
   *     replaced, or efficiently updated if the same result type was previous
   *     rendered there.
   * @param options RenderOptions for the entire render tree rendered to this
   *     container. Render options must *not* change between renders to the same
   *     container, as those changes will not effect previously rendered DOM.
   */

  const render = (result, container, options) => {
    let part = parts.get(container);

    if (part === undefined) {
      removeNodes(container, container.firstChild);
      parts.set(container, part = new NodePart(Object.assign({
        templateFactory
      }, options)));
      part.appendInto(container);
    }

    part.setValue(result);
    part.commit();
  };
  /**
   * @license
   * Copyright (c) 2017 The Polymer Project Authors. All rights reserved.
   * This code may only be used under the BSD style license found at
   * http://polymer.github.io/LICENSE.txt
   * The complete set of authors may be found at
   * http://polymer.github.io/AUTHORS.txt
   * The complete set of contributors may be found at
   * http://polymer.github.io/CONTRIBUTORS.txt
   * Code distributed by Google as part of the polymer project is also
   * subject to an additional IP rights grant found at
   * http://polymer.github.io/PATENTS.txt
   */
  // IMPORTANT: do not change the property name or the assignment expression.
  // This line will be used in regexes to search for lit-html usage.
  // TODO(justinfagnani): inject version number at build time


  (window['litHtmlVersions'] || (window['litHtmlVersions'] = [])).push('1.1.2');
  /**
   * Interprets a template literal as an HTML template that can efficiently
   * render to and update a container.
   */

  const html = (strings, ...values) => new TemplateResult(strings, values, 'html', defaultTemplateProcessor);
  /**
   * Interprets a template literal as an SVG template that can efficiently
   * render to and update a container.
   */


  const svg = (strings, ...values) => new SVGTemplateResult(strings, values, 'svg', defaultTemplateProcessor);
  /**
   * @license
   * Copyright (c) 2017 The Polymer Project Authors. All rights reserved.
   * This code may only be used under the BSD style license found at
   * http://polymer.github.io/LICENSE.txt
   * The complete set of authors may be found at
   * http://polymer.github.io/AUTHORS.txt
   * The complete set of contributors may be found at
   * http://polymer.github.io/CONTRIBUTORS.txt
   * Code distributed by Google as part of the polymer project is also
   * subject to an additional IP rights grant found at
   * http://polymer.github.io/PATENTS.txt
   */


  const walkerNodeFilter = 133
  /* NodeFilter.SHOW_{ELEMENT|COMMENT|TEXT} */
  ;
  /**
   * Removes the list of nodes from a Template safely. In addition to removing
   * nodes from the Template, the Template part indices are updated to match
   * the mutated Template DOM.
   *
   * As the template is walked the removal state is tracked and
   * part indices are adjusted as needed.
   *
   * div
   *   div#1 (remove) <-- start removing (removing node is div#1)
   *     div
   *       div#2 (remove)  <-- continue removing (removing node is still div#1)
   *         div
   * div <-- stop removing since previous sibling is the removing node (div#1,
   * removed 4 nodes)
   */

  function removeNodesFromTemplate(template, nodesToRemove) {
    const {
      element: {
        content
      },
      parts
    } = template;
    const walker = document.createTreeWalker(content, walkerNodeFilter, null, false);
    let partIndex = nextActiveIndexInTemplateParts(parts);
    let part = parts[partIndex];
    let nodeIndex = -1;
    let removeCount = 0;
    const nodesToRemoveInTemplate = [];
    let currentRemovingNode = null;

    while (walker.nextNode()) {
      nodeIndex++;
      const node = walker.currentNode; // End removal if stepped past the removing node

      if (node.previousSibling === currentRemovingNode) {
        currentRemovingNode = null;
      } // A node to remove was found in the template


      if (nodesToRemove.has(node)) {
        nodesToRemoveInTemplate.push(node); // Track node we're removing

        if (currentRemovingNode === null) {
          currentRemovingNode = node;
        }
      } // When removing, increment count by which to adjust subsequent part indices


      if (currentRemovingNode !== null) {
        removeCount++;
      }

      while (part !== undefined && part.index === nodeIndex) {
        // If part is in a removed node deactivate it by setting index to -1 or
        // adjust the index as needed.
        part.index = currentRemovingNode !== null ? -1 : part.index - removeCount; // go to the next active part.

        partIndex = nextActiveIndexInTemplateParts(parts, partIndex);
        part = parts[partIndex];
      }
    }

    nodesToRemoveInTemplate.forEach(n => n.parentNode.removeChild(n));
  }

  const countNodes = node => {
    let count = node.nodeType === 11
    /* Node.DOCUMENT_FRAGMENT_NODE */
    ? 0 : 1;
    const walker = document.createTreeWalker(node, walkerNodeFilter, null, false);

    while (walker.nextNode()) {
      count++;
    }

    return count;
  };

  const nextActiveIndexInTemplateParts = (parts, startIndex = -1) => {
    for (let i = startIndex + 1; i < parts.length; i++) {
      const part = parts[i];

      if (isTemplatePartActive(part)) {
        return i;
      }
    }

    return -1;
  };
  /**
   * Inserts the given node into the Template, optionally before the given
   * refNode. In addition to inserting the node into the Template, the Template
   * part indices are updated to match the mutated Template DOM.
   */


  function insertNodeIntoTemplate(template, node, refNode = null) {
    const {
      element: {
        content
      },
      parts
    } = template; // If there's no refNode, then put node at end of template.
    // No part indices need to be shifted in this case.

    if (refNode === null || refNode === undefined) {
      content.appendChild(node);
      return;
    }

    const walker = document.createTreeWalker(content, walkerNodeFilter, null, false);
    let partIndex = nextActiveIndexInTemplateParts(parts);
    let insertCount = 0;
    let walkerIndex = -1;

    while (walker.nextNode()) {
      walkerIndex++;
      const walkerNode = walker.currentNode;

      if (walkerNode === refNode) {
        insertCount = countNodes(node);
        refNode.parentNode.insertBefore(node, refNode);
      }

      while (partIndex !== -1 && parts[partIndex].index === walkerIndex) {
        // If we've inserted the node, simply adjust all subsequent parts
        if (insertCount > 0) {
          while (partIndex !== -1) {
            parts[partIndex].index += insertCount;
            partIndex = nextActiveIndexInTemplateParts(parts, partIndex);
          }

          return;
        }

        partIndex = nextActiveIndexInTemplateParts(parts, partIndex);
      }
    }
  }
  /**
   * @license
   * Copyright (c) 2017 The Polymer Project Authors. All rights reserved.
   * This code may only be used under the BSD style license found at
   * http://polymer.github.io/LICENSE.txt
   * The complete set of authors may be found at
   * http://polymer.github.io/AUTHORS.txt
   * The complete set of contributors may be found at
   * http://polymer.github.io/CONTRIBUTORS.txt
   * Code distributed by Google as part of the polymer project is also
   * subject to an additional IP rights grant found at
   * http://polymer.github.io/PATENTS.txt
   */
  // Get a key to lookup in `templateCaches`.


  const getTemplateCacheKey = (type, scopeName) => `${type}--${scopeName}`;

  let compatibleShadyCSSVersion = true;

  if (typeof window.ShadyCSS === 'undefined') {
    compatibleShadyCSSVersion = false;
  } else if (typeof window.ShadyCSS.prepareTemplateDom === 'undefined') {
    console.warn(`Incompatible ShadyCSS version detected. ` + `Please update to at least @webcomponents/webcomponentsjs@2.0.2 and ` + `@webcomponents/shadycss@1.3.1.`);
    compatibleShadyCSSVersion = false;
  }
  /**
   * Template factory which scopes template DOM using ShadyCSS.
   * @param scopeName {string}
   */


  const shadyTemplateFactory = scopeName => result => {
    const cacheKey = getTemplateCacheKey(result.type, scopeName);
    let templateCache = templateCaches.get(cacheKey);

    if (templateCache === undefined) {
      templateCache = {
        stringsArray: new WeakMap(),
        keyString: new Map()
      };
      templateCaches.set(cacheKey, templateCache);
    }

    let template = templateCache.stringsArray.get(result.strings);

    if (template !== undefined) {
      return template;
    }

    const key = result.strings.join(marker);
    template = templateCache.keyString.get(key);

    if (template === undefined) {
      const element = result.getTemplateElement();

      if (compatibleShadyCSSVersion) {
        window.ShadyCSS.prepareTemplateDom(element, scopeName);
      }

      template = new Template(result, element);
      templateCache.keyString.set(key, template);
    }

    templateCache.stringsArray.set(result.strings, template);
    return template;
  };

  const TEMPLATE_TYPES = ['html', 'svg'];
  /**
   * Removes all style elements from Templates for the given scopeName.
   */

  const removeStylesFromLitTemplates = scopeName => {
    TEMPLATE_TYPES.forEach(type => {
      const templates = templateCaches.get(getTemplateCacheKey(type, scopeName));

      if (templates !== undefined) {
        templates.keyString.forEach(template => {
          const {
            element: {
              content
            }
          } = template; // IE 11 doesn't support the iterable param Set constructor

          const styles = new Set();
          Array.from(content.querySelectorAll('style')).forEach(s => {
            styles.add(s);
          });
          removeNodesFromTemplate(template, styles);
        });
      }
    });
  };

  const shadyRenderSet = new Set();
  /**
   * For the given scope name, ensures that ShadyCSS style scoping is performed.
   * This is done just once per scope name so the fragment and template cannot
   * be modified.
   * (1) extracts styles from the rendered fragment and hands them to ShadyCSS
   * to be scoped and appended to the document
   * (2) removes style elements from all lit-html Templates for this scope name.
   *
   * Note, <style> elements can only be placed into templates for the
   * initial rendering of the scope. If <style> elements are included in templates
   * dynamically rendered to the scope (after the first scope render), they will
   * not be scoped and the <style> will be left in the template and rendered
   * output.
   */

  const prepareTemplateStyles = (scopeName, renderedDOM, template) => {
    shadyRenderSet.add(scopeName); // If `renderedDOM` is stamped from a Template, then we need to edit that
    // Template's underlying template element. Otherwise, we create one here
    // to give to ShadyCSS, which still requires one while scoping.

    const templateElement = !!template ? template.element : document.createElement('template'); // Move styles out of rendered DOM and store.

    const styles = renderedDOM.querySelectorAll('style');
    const {
      length
    } = styles; // If there are no styles, skip unnecessary work

    if (length === 0) {
      // Ensure prepareTemplateStyles is called to support adding
      // styles via `prepareAdoptedCssText` since that requires that
      // `prepareTemplateStyles` is called.
      //
      // ShadyCSS will only update styles containing @apply in the template
      // given to `prepareTemplateStyles`. If no lit Template was given,
      // ShadyCSS will not be able to update uses of @apply in any relevant
      // template. However, this is not a problem because we only create the
      // template for the purpose of supporting `prepareAdoptedCssText`,
      // which doesn't support @apply at all.
      window.ShadyCSS.prepareTemplateStyles(templateElement, scopeName);
      return;
    }

    const condensedStyle = document.createElement('style'); // Collect styles into a single style. This helps us make sure ShadyCSS
    // manipulations will not prevent us from being able to fix up template
    // part indices.
    // NOTE: collecting styles is inefficient for browsers but ShadyCSS
    // currently does this anyway. When it does not, this should be changed.

    for (let i = 0; i < length; i++) {
      const style = styles[i];
      style.parentNode.removeChild(style);
      condensedStyle.textContent += style.textContent;
    } // Remove styles from nested templates in this scope.


    removeStylesFromLitTemplates(scopeName); // And then put the condensed style into the "root" template passed in as
    // `template`.

    const content = templateElement.content;

    if (!!template) {
      insertNodeIntoTemplate(template, condensedStyle, content.firstChild);
    } else {
      content.insertBefore(condensedStyle, content.firstChild);
    } // Note, it's important that ShadyCSS gets the template that `lit-html`
    // will actually render so that it can update the style inside when
    // needed (e.g. @apply native Shadow DOM case).


    window.ShadyCSS.prepareTemplateStyles(templateElement, scopeName);
    const style = content.querySelector('style');

    if (window.ShadyCSS.nativeShadow && style !== null) {
      // When in native Shadow DOM, ensure the style created by ShadyCSS is
      // included in initially rendered output (`renderedDOM`).
      renderedDOM.insertBefore(style.cloneNode(true), renderedDOM.firstChild);
    } else if (!!template) {
      // When no style is left in the template, parts will be broken as a
      // result. To fix this, we put back the style node ShadyCSS removed
      // and then tell lit to remove that node from the template.
      // There can be no style in the template in 2 cases (1) when Shady DOM
      // is in use, ShadyCSS removes all styles, (2) when native Shadow DOM
      // is in use ShadyCSS removes the style if it contains no content.
      // NOTE, ShadyCSS creates its own style so we can safely add/remove
      // `condensedStyle` here.
      content.insertBefore(condensedStyle, content.firstChild);
      const removes = new Set();
      removes.add(condensedStyle);
      removeNodesFromTemplate(template, removes);
    }
  };
  /**
   * Extension to the standard `render` method which supports rendering
   * to ShadowRoots when the ShadyDOM (https://github.com/webcomponents/shadydom)
   * and ShadyCSS (https://github.com/webcomponents/shadycss) polyfills are used
   * or when the webcomponentsjs
   * (https://github.com/webcomponents/webcomponentsjs) polyfill is used.
   *
   * Adds a `scopeName` option which is used to scope element DOM and stylesheets
   * when native ShadowDOM is unavailable. The `scopeName` will be added to
   * the class attribute of all rendered DOM. In addition, any style elements will
   * be automatically re-written with this `scopeName` selector and moved out
   * of the rendered DOM and into the document `<head>`.
   *
   * It is common to use this render method in conjunction with a custom element
   * which renders a shadowRoot. When this is done, typically the element's
   * `localName` should be used as the `scopeName`.
   *
   * In addition to DOM scoping, ShadyCSS also supports a basic shim for css
   * custom properties (needed only on older browsers like IE11) and a shim for
   * a deprecated feature called `@apply` that supports applying a set of css
   * custom properties to a given location.
   *
   * Usage considerations:
   *
   * * Part values in `<style>` elements are only applied the first time a given
   * `scopeName` renders. Subsequent changes to parts in style elements will have
   * no effect. Because of this, parts in style elements should only be used for
   * values that will never change, for example parts that set scope-wide theme
   * values or parts which render shared style elements.
   *
   * * Note, due to a limitation of the ShadyDOM polyfill, rendering in a
   * custom element's `constructor` is not supported. Instead rendering should
   * either done asynchronously, for example at microtask timing (for example
   * `Promise.resolve()`), or be deferred until the first time the element's
   * `connectedCallback` runs.
   *
   * Usage considerations when using shimmed custom properties or `@apply`:
   *
   * * Whenever any dynamic changes are made which affect
   * css custom properties, `ShadyCSS.styleElement(element)` must be called
   * to update the element. There are two cases when this is needed:
   * (1) the element is connected to a new parent, (2) a class is added to the
   * element that causes it to match different custom properties.
   * To address the first case when rendering a custom element, `styleElement`
   * should be called in the element's `connectedCallback`.
   *
   * * Shimmed custom properties may only be defined either for an entire
   * shadowRoot (for example, in a `:host` rule) or via a rule that directly
   * matches an element with a shadowRoot. In other words, instead of flowing from
   * parent to child as do native css custom properties, shimmed custom properties
   * flow only from shadowRoots to nested shadowRoots.
   *
   * * When using `@apply` mixing css shorthand property names with
   * non-shorthand names (for example `border` and `border-width`) is not
   * supported.
   */


  const render$1 = (result, container, options) => {
    if (!options || typeof options !== 'object' || !options.scopeName) {
      throw new Error('The `scopeName` option is required.');
    }

    const scopeName = options.scopeName;
    const hasRendered = parts.has(container);
    const needsScoping = compatibleShadyCSSVersion && container.nodeType === 11
    /* Node.DOCUMENT_FRAGMENT_NODE */
    && !!container.host; // Handle first render to a scope specially...

    const firstScopeRender = needsScoping && !shadyRenderSet.has(scopeName); // On first scope render, render into a fragment; this cannot be a single
    // fragment that is reused since nested renders can occur synchronously.

    const renderContainer = firstScopeRender ? document.createDocumentFragment() : container;
    render(result, renderContainer, Object.assign({
      templateFactory: shadyTemplateFactory(scopeName)
    }, options)); // When performing first scope render,
    // (1) We've rendered into a fragment so that there's a chance to
    // `prepareTemplateStyles` before sub-elements hit the DOM
    // (which might cause them to render based on a common pattern of
    // rendering in a custom element's `connectedCallback`);
    // (2) Scope the template with ShadyCSS one time only for this scope.
    // (3) Render the fragment into the container and make sure the
    // container knows its `part` is the one we just rendered. This ensures
    // DOM will be re-used on subsequent renders.

    if (firstScopeRender) {
      const part = parts.get(renderContainer);
      parts.delete(renderContainer); // ShadyCSS might have style sheets (e.g. from `prepareAdoptedCssText`)
      // that should apply to `renderContainer` even if the rendered value is
      // not a TemplateInstance. However, it will only insert scoped styles
      // into the document if `prepareTemplateStyles` has already been called
      // for the given scope name.

      const template = part.value instanceof TemplateInstance ? part.value.template : undefined;
      prepareTemplateStyles(scopeName, renderContainer, template);
      removeNodes(container, container.firstChild);
      container.appendChild(renderContainer);
      parts.set(container, part);
    } // After elements have hit the DOM, update styling if this is the
    // initial render to this container.
    // This is needed whenever dynamic changes are made so it would be
    // safest to do every render; however, this would regress performance
    // so we leave it up to the user to call `ShadyCSS.styleElement`
    // for dynamic changes.


    if (!hasRendered && needsScoping) {
      window.ShadyCSS.styleElement(container.host);
    }
  };
  /**
   * @license
   * Copyright (c) 2017 The Polymer Project Authors. All rights reserved.
   * This code may only be used under the BSD style license found at
   * http://polymer.github.io/LICENSE.txt
   * The complete set of authors may be found at
   * http://polymer.github.io/AUTHORS.txt
   * The complete set of contributors may be found at
   * http://polymer.github.io/CONTRIBUTORS.txt
   * Code distributed by Google as part of the polymer project is also
   * subject to an additional IP rights grant found at
   * http://polymer.github.io/PATENTS.txt
   */


  var _a;
  /**
   * When using Closure Compiler, JSCompiler_renameProperty(property, object) is
   * replaced at compile time by the munged name for object[property]. We cannot
   * alias this function, so we have to use a small shim that has the same
   * behavior when not compiling.
   */


  window.JSCompiler_renameProperty = (prop, _obj) => prop;

  const defaultConverter = {
    toAttribute(value, type) {
      switch (type) {
        case Boolean:
          return value ? '' : null;

        case Object:
        case Array:
          // if the value is `null` or `undefined` pass this through
          // to allow removing/no change behavior.
          return value == null ? value : JSON.stringify(value);
      }

      return value;
    },

    fromAttribute(value, type) {
      switch (type) {
        case Boolean:
          return value !== null;

        case Number:
          return value === null ? null : Number(value);

        case Object:
        case Array:
          return JSON.parse(value);
      }

      return value;
    }

  };
  /**
   * Change function that returns true if `value` is different from `oldValue`.
   * This method is used as the default for a property's `hasChanged` function.
   */

  const notEqual = (value, old) => {
    // This ensures (old==NaN, value==NaN) always returns false
    return old !== value && (old === old || value === value);
  };

  const defaultPropertyDeclaration = {
    attribute: true,
    type: String,
    converter: defaultConverter,
    reflect: false,
    hasChanged: notEqual
  };
  const microtaskPromise = Promise.resolve(true);
  const STATE_HAS_UPDATED = 1;
  const STATE_UPDATE_REQUESTED = 1 << 2;
  const STATE_IS_REFLECTING_TO_ATTRIBUTE = 1 << 3;
  const STATE_IS_REFLECTING_TO_PROPERTY = 1 << 4;
  const STATE_HAS_CONNECTED = 1 << 5;
  /**
   * The Closure JS Compiler doesn't currently have good support for static
   * property semantics where "this" is dynamic (e.g.
   * https://github.com/google/closure-compiler/issues/3177 and others) so we use
   * this hack to bypass any rewriting by the compiler.
   */

  const finalized = 'finalized';
  /**
   * Base element class which manages element properties and attributes. When
   * properties change, the `update` method is asynchronously called. This method
   * should be supplied by subclassers to render updates as desired.
   */

  class UpdatingElement extends HTMLElement {
    constructor() {
      super();
      this._updateState = 0;
      this._instanceProperties = undefined;
      this._updatePromise = microtaskPromise;
      this._hasConnectedResolver = undefined;
      /**
       * Map with keys for any properties that have changed since the last
       * update cycle with previous values.
       */

      this._changedProperties = new Map();
      /**
       * Map with keys of properties that should be reflected when updated.
       */

      this._reflectingProperties = undefined;
      this.initialize();
    }
    /**
     * Returns a list of attributes corresponding to the registered properties.
     * @nocollapse
     */


    static get observedAttributes() {
      // note: piggy backing on this to ensure we're finalized.
      this.finalize();
      const attributes = []; // Use forEach so this works even if for/of loops are compiled to for loops
      // expecting arrays

      this._classProperties.forEach((v, p) => {
        const attr = this._attributeNameForProperty(p, v);

        if (attr !== undefined) {
          this._attributeToPropertyMap.set(attr, p);

          attributes.push(attr);
        }
      });

      return attributes;
    }
    /**
     * Ensures the private `_classProperties` property metadata is created.
     * In addition to `finalize` this is also called in `createProperty` to
     * ensure the `@property` decorator can add property metadata.
     */

    /** @nocollapse */


    static _ensureClassProperties() {
      // ensure private storage for property declarations.
      if (!this.hasOwnProperty(JSCompiler_renameProperty('_classProperties', this))) {
        this._classProperties = new Map(); // NOTE: Workaround IE11 not supporting Map constructor argument.

        const superProperties = Object.getPrototypeOf(this)._classProperties;

        if (superProperties !== undefined) {
          superProperties.forEach((v, k) => this._classProperties.set(k, v));
        }
      }
    }
    /**
     * Creates a property accessor on the element prototype if one does not exist.
     * The property setter calls the property's `hasChanged` property option
     * or uses a strict identity check to determine whether or not to request
     * an update.
     * @nocollapse
     */


    static createProperty(name, options = defaultPropertyDeclaration) {
      // Note, since this can be called by the `@property` decorator which
      // is called before `finalize`, we ensure storage exists for property
      // metadata.
      this._ensureClassProperties();

      this._classProperties.set(name, options); // Do not generate an accessor if the prototype already has one, since
      // it would be lost otherwise and that would never be the user's intention;
      // Instead, we expect users to call `requestUpdate` themselves from
      // user-defined accessors. Note that if the super has an accessor we will
      // still overwrite it


      if (options.noAccessor || this.prototype.hasOwnProperty(name)) {
        return;
      }

      const key = typeof name === 'symbol' ? Symbol() : `__${name}`;
      Object.defineProperty(this.prototype, name, {
        // tslint:disable-next-line:no-any no symbol in index
        get() {
          return this[key];
        },

        set(value) {
          const oldValue = this[name];
          this[key] = value;

          this._requestUpdate(name, oldValue);
        },

        configurable: true,
        enumerable: true
      });
    }
    /**
     * Creates property accessors for registered properties and ensures
     * any superclasses are also finalized.
     * @nocollapse
     */


    static finalize() {
      // finalize any superclasses
      const superCtor = Object.getPrototypeOf(this);

      if (!superCtor.hasOwnProperty(finalized)) {
        superCtor.finalize();
      }

      this[finalized] = true;

      this._ensureClassProperties(); // initialize Map populated in observedAttributes


      this._attributeToPropertyMap = new Map(); // make any properties
      // Note, only process "own" properties since this element will inherit
      // any properties defined on the superClass, and finalization ensures
      // the entire prototype chain is finalized.

      if (this.hasOwnProperty(JSCompiler_renameProperty('properties', this))) {
        const props = this.properties; // support symbols in properties (IE11 does not support this)

        const propKeys = [...Object.getOwnPropertyNames(props), ...(typeof Object.getOwnPropertySymbols === 'function' ? Object.getOwnPropertySymbols(props) : [])]; // This for/of is ok because propKeys is an array

        for (const p of propKeys) {
          // note, use of `any` is due to TypeSript lack of support for symbol in
          // index types
          // tslint:disable-next-line:no-any no symbol in index
          this.createProperty(p, props[p]);
        }
      }
    }
    /**
     * Returns the property name for the given attribute `name`.
     * @nocollapse
     */


    static _attributeNameForProperty(name, options) {
      const attribute = options.attribute;
      return attribute === false ? undefined : typeof attribute === 'string' ? attribute : typeof name === 'string' ? name.toLowerCase() : undefined;
    }
    /**
     * Returns true if a property should request an update.
     * Called when a property value is set and uses the `hasChanged`
     * option for the property if present or a strict identity check.
     * @nocollapse
     */


    static _valueHasChanged(value, old, hasChanged = notEqual) {
      return hasChanged(value, old);
    }
    /**
     * Returns the property value for the given attribute value.
     * Called via the `attributeChangedCallback` and uses the property's
     * `converter` or `converter.fromAttribute` property option.
     * @nocollapse
     */


    static _propertyValueFromAttribute(value, options) {
      const type = options.type;
      const converter = options.converter || defaultConverter;
      const fromAttribute = typeof converter === 'function' ? converter : converter.fromAttribute;
      return fromAttribute ? fromAttribute(value, type) : value;
    }
    /**
     * Returns the attribute value for the given property value. If this
     * returns undefined, the property will *not* be reflected to an attribute.
     * If this returns null, the attribute will be removed, otherwise the
     * attribute will be set to the value.
     * This uses the property's `reflect` and `type.toAttribute` property options.
     * @nocollapse
     */


    static _propertyValueToAttribute(value, options) {
      if (options.reflect === undefined) {
        return;
      }

      const type = options.type;
      const converter = options.converter;
      const toAttribute = converter && converter.toAttribute || defaultConverter.toAttribute;
      return toAttribute(value, type);
    }
    /**
     * Performs element initialization. By default captures any pre-set values for
     * registered properties.
     */


    initialize() {
      this._saveInstanceProperties(); // ensures first update will be caught by an early access of
      // `updateComplete`


      this._requestUpdate();
    }
    /**
     * Fixes any properties set on the instance before upgrade time.
     * Otherwise these would shadow the accessor and break these properties.
     * The properties are stored in a Map which is played back after the
     * constructor runs. Note, on very old versions of Safari (<=9) or Chrome
     * (<=41), properties created for native platform properties like (`id` or
     * `name`) may not have default values set in the element constructor. On
     * these browsers native properties appear on instances and therefore their
     * default value will overwrite any element default (e.g. if the element sets
     * this.id = 'id' in the constructor, the 'id' will become '' since this is
     * the native platform default).
     */


    _saveInstanceProperties() {
      // Use forEach so this works even if for/of loops are compiled to for loops
      // expecting arrays
      this.constructor._classProperties.forEach((_v, p) => {
        if (this.hasOwnProperty(p)) {
          const value = this[p];
          delete this[p];

          if (!this._instanceProperties) {
            this._instanceProperties = new Map();
          }

          this._instanceProperties.set(p, value);
        }
      });
    }
    /**
     * Applies previously saved instance properties.
     */


    _applyInstanceProperties() {
      // Use forEach so this works even if for/of loops are compiled to for loops
      // expecting arrays
      // tslint:disable-next-line:no-any
      this._instanceProperties.forEach((v, p) => this[p] = v);

      this._instanceProperties = undefined;
    }

    connectedCallback() {
      this._updateState = this._updateState | STATE_HAS_CONNECTED; // Ensure first connection completes an update. Updates cannot complete
      // before connection and if one is pending connection the
      // `_hasConnectionResolver` will exist. If so, resolve it to complete the
      // update, otherwise requestUpdate.

      if (this._hasConnectedResolver) {
        this._hasConnectedResolver();

        this._hasConnectedResolver = undefined;
      }
    }
    /**
     * Allows for `super.disconnectedCallback()` in extensions while
     * reserving the possibility of making non-breaking feature additions
     * when disconnecting at some point in the future.
     */


    disconnectedCallback() {}
    /**
     * Synchronizes property values when attributes change.
     */


    attributeChangedCallback(name, old, value) {
      if (old !== value) {
        this._attributeToProperty(name, value);
      }
    }

    _propertyToAttribute(name, value, options = defaultPropertyDeclaration) {
      const ctor = this.constructor;

      const attr = ctor._attributeNameForProperty(name, options);

      if (attr !== undefined) {
        const attrValue = ctor._propertyValueToAttribute(value, options); // an undefined value does not change the attribute.


        if (attrValue === undefined) {
          return;
        } // Track if the property is being reflected to avoid
        // setting the property again via `attributeChangedCallback`. Note:
        // 1. this takes advantage of the fact that the callback is synchronous.
        // 2. will behave incorrectly if multiple attributes are in the reaction
        // stack at time of calling. However, since we process attributes
        // in `update` this should not be possible (or an extreme corner case
        // that we'd like to discover).
        // mark state reflecting


        this._updateState = this._updateState | STATE_IS_REFLECTING_TO_ATTRIBUTE;

        if (attrValue == null) {
          this.removeAttribute(attr);
        } else {
          this.setAttribute(attr, attrValue);
        } // mark state not reflecting


        this._updateState = this._updateState & ~STATE_IS_REFLECTING_TO_ATTRIBUTE;
      }
    }

    _attributeToProperty(name, value) {
      // Use tracking info to avoid deserializing attribute value if it was
      // just set from a property setter.
      if (this._updateState & STATE_IS_REFLECTING_TO_ATTRIBUTE) {
        return;
      }

      const ctor = this.constructor;

      const propName = ctor._attributeToPropertyMap.get(name);

      if (propName !== undefined) {
        const options = ctor._classProperties.get(propName) || defaultPropertyDeclaration; // mark state reflecting

        this._updateState = this._updateState | STATE_IS_REFLECTING_TO_PROPERTY;
        this[propName] = // tslint:disable-next-line:no-any
        ctor._propertyValueFromAttribute(value, options); // mark state not reflecting

        this._updateState = this._updateState & ~STATE_IS_REFLECTING_TO_PROPERTY;
      }
    }
    /**
     * This private version of `requestUpdate` does not access or return the
     * `updateComplete` promise. This promise can be overridden and is therefore
     * not free to access.
     */


    _requestUpdate(name, oldValue) {
      let shouldRequestUpdate = true; // If we have a property key, perform property update steps.

      if (name !== undefined) {
        const ctor = this.constructor;
        const options = ctor._classProperties.get(name) || defaultPropertyDeclaration;

        if (ctor._valueHasChanged(this[name], oldValue, options.hasChanged)) {
          if (!this._changedProperties.has(name)) {
            this._changedProperties.set(name, oldValue);
          } // Add to reflecting properties set.
          // Note, it's important that every change has a chance to add the
          // property to `_reflectingProperties`. This ensures setting
          // attribute + property reflects correctly.


          if (options.reflect === true && !(this._updateState & STATE_IS_REFLECTING_TO_PROPERTY)) {
            if (this._reflectingProperties === undefined) {
              this._reflectingProperties = new Map();
            }

            this._reflectingProperties.set(name, options);
          }
        } else {
          // Abort the request if the property should not be considered changed.
          shouldRequestUpdate = false;
        }
      }

      if (!this._hasRequestedUpdate && shouldRequestUpdate) {
        this._enqueueUpdate();
      }
    }
    /**
     * Requests an update which is processed asynchronously. This should
     * be called when an element should update based on some state not triggered
     * by setting a property. In this case, pass no arguments. It should also be
     * called when manually implementing a property setter. In this case, pass the
     * property `name` and `oldValue` to ensure that any configured property
     * options are honored. Returns the `updateComplete` Promise which is resolved
     * when the update completes.
     *
     * @param name {PropertyKey} (optional) name of requesting property
     * @param oldValue {any} (optional) old value of requesting property
     * @returns {Promise} A Promise that is resolved when the update completes.
     */


    requestUpdate(name, oldValue) {
      this._requestUpdate(name, oldValue);

      return this.updateComplete;
    }
    /**
     * Sets up the element to asynchronously update.
     */


    async _enqueueUpdate() {
      // Mark state updating...
      this._updateState = this._updateState | STATE_UPDATE_REQUESTED;
      let resolve;
      let reject;
      const previousUpdatePromise = this._updatePromise;
      this._updatePromise = new Promise((res, rej) => {
        resolve = res;
        reject = rej;
      });

      try {
        // Ensure any previous update has resolved before updating.
        // This `await` also ensures that property changes are batched.
        await previousUpdatePromise;
      } catch (e) {} // Ignore any previous errors. We only care that the previous cycle is
      // done. Any error should have been handled in the previous update.
      // Make sure the element has connected before updating.


      if (!this._hasConnected) {
        await new Promise(res => this._hasConnectedResolver = res);
      }

      try {
        const result = this.performUpdate(); // If `performUpdate` returns a Promise, we await it. This is done to
        // enable coordinating updates with a scheduler. Note, the result is
        // checked to avoid delaying an additional microtask unless we need to.

        if (result != null) {
          await result;
        }
      } catch (e) {
        reject(e);
      }

      resolve(!this._hasRequestedUpdate);
    }

    get _hasConnected() {
      return this._updateState & STATE_HAS_CONNECTED;
    }

    get _hasRequestedUpdate() {
      return this._updateState & STATE_UPDATE_REQUESTED;
    }

    get hasUpdated() {
      return this._updateState & STATE_HAS_UPDATED;
    }
    /**
     * Performs an element update. Note, if an exception is thrown during the
     * update, `firstUpdated` and `updated` will not be called.
     *
     * You can override this method to change the timing of updates. If this
     * method is overridden, `super.performUpdate()` must be called.
     *
     * For instance, to schedule updates to occur just before the next frame:
     *
     * ```
     * protected async performUpdate(): Promise<unknown> {
     *   await new Promise((resolve) => requestAnimationFrame(() => resolve()));
     *   super.performUpdate();
     * }
     * ```
     */


    performUpdate() {
      // Mixin instance properties once, if they exist.
      if (this._instanceProperties) {
        this._applyInstanceProperties();
      }

      let shouldUpdate = false;
      const changedProperties = this._changedProperties;

      try {
        shouldUpdate = this.shouldUpdate(changedProperties);

        if (shouldUpdate) {
          this.update(changedProperties);
        }
      } catch (e) {
        // Prevent `firstUpdated` and `updated` from running when there's an
        // update exception.
        shouldUpdate = false;
        throw e;
      } finally {
        // Ensure element can accept additional updates after an exception.
        this._markUpdated();
      }

      if (shouldUpdate) {
        if (!(this._updateState & STATE_HAS_UPDATED)) {
          this._updateState = this._updateState | STATE_HAS_UPDATED;
          this.firstUpdated(changedProperties);
        }

        this.updated(changedProperties);
      }
    }

    _markUpdated() {
      this._changedProperties = new Map();
      this._updateState = this._updateState & ~STATE_UPDATE_REQUESTED;
    }
    /**
     * Returns a Promise that resolves when the element has completed updating.
     * The Promise value is a boolean that is `true` if the element completed the
     * update without triggering another update. The Promise result is `false` if
     * a property was set inside `updated()`. If the Promise is rejected, an
     * exception was thrown during the update.
     *
     * To await additional asynchronous work, override the `_getUpdateComplete`
     * method. For example, it is sometimes useful to await a rendered element
     * before fulfilling this Promise. To do this, first await
     * `super._getUpdateComplete()`, then any subsequent state.
     *
     * @returns {Promise} The Promise returns a boolean that indicates if the
     * update resolved without triggering another update.
     */


    get updateComplete() {
      return this._getUpdateComplete();
    }
    /**
     * Override point for the `updateComplete` promise.
     *
     * It is not safe to override the `updateComplete` getter directly due to a
     * limitation in TypeScript which means it is not possible to call a
     * superclass getter (e.g. `super.updateComplete.then(...)`) when the target
     * language is ES5 (https://github.com/microsoft/TypeScript/issues/338).
     * This method should be overridden instead. For example:
     *
     *   class MyElement extends LitElement {
     *     async _getUpdateComplete() {
     *       await super._getUpdateComplete();
     *       await this._myChild.updateComplete;
     *     }
     *   }
     */


    _getUpdateComplete() {
      return this._updatePromise;
    }
    /**
     * Controls whether or not `update` should be called when the element requests
     * an update. By default, this method always returns `true`, but this can be
     * customized to control when to update.
     *
     * * @param _changedProperties Map of changed properties with old values
     */


    shouldUpdate(_changedProperties) {
      return true;
    }
    /**
     * Updates the element. This method reflects property values to attributes.
     * It can be overridden to render and keep updated element DOM.
     * Setting properties inside this method will *not* trigger
     * another update.
     *
     * * @param _changedProperties Map of changed properties with old values
     */


    update(_changedProperties) {
      if (this._reflectingProperties !== undefined && this._reflectingProperties.size > 0) {
        // Use forEach so this works even if for/of loops are compiled to for
        // loops expecting arrays
        this._reflectingProperties.forEach((v, k) => this._propertyToAttribute(k, this[k], v));

        this._reflectingProperties = undefined;
      }
    }
    /**
     * Invoked whenever the element is updated. Implement to perform
     * post-updating tasks via DOM APIs, for example, focusing an element.
     *
     * Setting properties inside this method will trigger the element to update
     * again after this update cycle completes.
     *
     * * @param _changedProperties Map of changed properties with old values
     */


    updated(_changedProperties) {}
    /**
     * Invoked when the element is first updated. Implement to perform one time
     * work on the element after update.
     *
     * Setting properties inside this method will trigger the element to update
     * again after this update cycle completes.
     *
     * * @param _changedProperties Map of changed properties with old values
     */


    firstUpdated(_changedProperties) {}

  }

  _a = finalized;
  /**
   * Marks class as having finished creating properties.
   */

  UpdatingElement[_a] = true;
  /**
   * @license
   * Copyright (c) 2017 The Polymer Project Authors. All rights reserved.
   * This code may only be used under the BSD style license found at
   * http://polymer.github.io/LICENSE.txt
   * The complete set of authors may be found at
   * http://polymer.github.io/AUTHORS.txt
   * The complete set of contributors may be found at
   * http://polymer.github.io/CONTRIBUTORS.txt
   * Code distributed by Google as part of the polymer project is also
   * subject to an additional IP rights grant found at
   * http://polymer.github.io/PATENTS.txt
   */

  /**
  @license
  Copyright (c) 2019 The Polymer Project Authors. All rights reserved.
  This code may only be used under the BSD style license found at
  http://polymer.github.io/LICENSE.txt The complete set of authors may be found at
  http://polymer.github.io/AUTHORS.txt The complete set of contributors may be
  found at http://polymer.github.io/CONTRIBUTORS.txt Code distributed by Google as
  part of the polymer project is also subject to an additional IP rights grant
  found at http://polymer.github.io/PATENTS.txt
  */

  const supportsAdoptingStyleSheets = 'adoptedStyleSheets' in Document.prototype && 'replace' in CSSStyleSheet.prototype;
  const constructionToken = Symbol();

  class CSSResult {
    constructor(cssText, safeToken) {
      if (safeToken !== constructionToken) {
        throw new Error('CSSResult is not constructable. Use `unsafeCSS` or `css` instead.');
      }

      this.cssText = cssText;
    } // Note, this is a getter so that it's lazy. In practice, this means
    // stylesheets are not created until the first element instance is made.


    get styleSheet() {
      if (this._styleSheet === undefined) {
        // Note, if `adoptedStyleSheets` is supported then we assume CSSStyleSheet
        // is constructable.
        if (supportsAdoptingStyleSheets) {
          this._styleSheet = new CSSStyleSheet();

          this._styleSheet.replaceSync(this.cssText);
        } else {
          this._styleSheet = null;
        }
      }

      return this._styleSheet;
    }

    toString() {
      return this.cssText;
    }

  }

  const textFromCSSResult = value => {
    if (value instanceof CSSResult) {
      return value.cssText;
    } else if (typeof value === 'number') {
      return value;
    } else {
      throw new Error(`Value passed to 'css' function must be a 'css' function result: ${value}. Use 'unsafeCSS' to pass non-literal values, but
            take care to ensure page security.`);
    }
  };
  /**
   * Template tag which which can be used with LitElement's `style` property to
   * set element styles. For security reasons, only literal string values may be
   * used. To incorporate non-literal values `unsafeCSS` may be used inside a
   * template string part.
   */


  const css = (strings, ...values) => {
    const cssText = values.reduce((acc, v, idx) => acc + textFromCSSResult(v) + strings[idx + 1], strings[0]);
    return new CSSResult(cssText, constructionToken);
  };
  /**
   * @license
   * Copyright (c) 2017 The Polymer Project Authors. All rights reserved.
   * This code may only be used under the BSD style license found at
   * http://polymer.github.io/LICENSE.txt
   * The complete set of authors may be found at
   * http://polymer.github.io/AUTHORS.txt
   * The complete set of contributors may be found at
   * http://polymer.github.io/CONTRIBUTORS.txt
   * Code distributed by Google as part of the polymer project is also
   * subject to an additional IP rights grant found at
   * http://polymer.github.io/PATENTS.txt
   */
  // IMPORTANT: do not change the property name or the assignment expression.
  // This line will be used in regexes to search for LitElement usage.
  // TODO(justinfagnani): inject version number at build time


  (window['litElementVersions'] || (window['litElementVersions'] = [])).push('2.2.1');
  /**
   * Minimal implementation of Array.prototype.flat
   * @param arr the array to flatten
   * @param result the accumlated result
   */

  function arrayFlat(styles, result = []) {
    for (let i = 0, length = styles.length; i < length; i++) {
      const value = styles[i];

      if (Array.isArray(value)) {
        arrayFlat(value, result);
      } else {
        result.push(value);
      }
    }

    return result;
  }
  /** Deeply flattens styles array. Uses native flat if available. */


  const flattenStyles = styles => styles.flat ? styles.flat(Infinity) : arrayFlat(styles);

  class LitElement extends UpdatingElement {
    /** @nocollapse */
    static finalize() {
      // The Closure JS Compiler does not always preserve the correct "this"
      // when calling static super methods (b/137460243), so explicitly bind.
      super.finalize.call(this); // Prepare styling that is stamped at first render time. Styling
      // is built from user provided `styles` or is inherited from the superclass.

      this._styles = this.hasOwnProperty(JSCompiler_renameProperty('styles', this)) ? this._getUniqueStyles() : this._styles || [];
    }
    /** @nocollapse */


    static _getUniqueStyles() {
      // Take care not to call `this.styles` multiple times since this generates
      // new CSSResults each time.
      // TODO(sorvell): Since we do not cache CSSResults by input, any
      // shared styles will generate new stylesheet objects, which is wasteful.
      // This should be addressed when a browser ships constructable
      // stylesheets.
      const userStyles = this.styles;
      const styles = [];

      if (Array.isArray(userStyles)) {
        const flatStyles = flattenStyles(userStyles); // As a performance optimization to avoid duplicated styling that can
        // occur especially when composing via subclassing, de-duplicate styles
        // preserving the last item in the list. The last item is kept to
        // try to preserve cascade order with the assumption that it's most
        // important that last added styles override previous styles.

        const styleSet = flatStyles.reduceRight((set, s) => {
          set.add(s); // on IE set.add does not return the set.

          return set;
        }, new Set()); // Array.from does not work on Set in IE

        styleSet.forEach(v => styles.unshift(v));
      } else if (userStyles) {
        styles.push(userStyles);
      }

      return styles;
    }
    /**
     * Performs element initialization. By default this calls `createRenderRoot`
     * to create the element `renderRoot` node and captures any pre-set values for
     * registered properties.
     */


    initialize() {
      super.initialize();
      this.renderRoot = this.createRenderRoot(); // Note, if renderRoot is not a shadowRoot, styles would/could apply to the
      // element's getRootNode(). While this could be done, we're choosing not to
      // support this now since it would require different logic around de-duping.

      if (window.ShadowRoot && this.renderRoot instanceof window.ShadowRoot) {
        this.adoptStyles();
      }
    }
    /**
     * Returns the node into which the element should render and by default
     * creates and returns an open shadowRoot. Implement to customize where the
     * element's DOM is rendered. For example, to render into the element's
     * childNodes, return `this`.
     * @returns {Element|DocumentFragment} Returns a node into which to render.
     */


    createRenderRoot() {
      return this.attachShadow({
        mode: 'open'
      });
    }
    /**
     * Applies styling to the element shadowRoot using the `static get styles`
     * property. Styling will apply using `shadowRoot.adoptedStyleSheets` where
     * available and will fallback otherwise. When Shadow DOM is polyfilled,
     * ShadyCSS scopes styles and adds them to the document. When Shadow DOM
     * is available but `adoptedStyleSheets` is not, styles are appended to the
     * end of the `shadowRoot` to [mimic spec
     * behavior](https://wicg.github.io/construct-stylesheets/#using-constructed-stylesheets).
     */


    adoptStyles() {
      const styles = this.constructor._styles;

      if (styles.length === 0) {
        return;
      } // There are three separate cases here based on Shadow DOM support.
      // (1) shadowRoot polyfilled: use ShadyCSS
      // (2) shadowRoot.adoptedStyleSheets available: use it.
      // (3) shadowRoot.adoptedStyleSheets polyfilled: append styles after
      // rendering


      if (window.ShadyCSS !== undefined && !window.ShadyCSS.nativeShadow) {
        window.ShadyCSS.ScopingShim.prepareAdoptedCssText(styles.map(s => s.cssText), this.localName);
      } else if (supportsAdoptingStyleSheets) {
        this.renderRoot.adoptedStyleSheets = styles.map(s => s.styleSheet);
      } else {
        // This must be done after rendering so the actual style insertion is done
        // in `update`.
        this._needsShimAdoptedStyleSheets = true;
      }
    }

    connectedCallback() {
      super.connectedCallback(); // Note, first update/render handles styleElement so we only call this if
      // connected after first update.

      if (this.hasUpdated && window.ShadyCSS !== undefined) {
        window.ShadyCSS.styleElement(this);
      }
    }
    /**
     * Updates the element. This method reflects property values to attributes
     * and calls `render` to render DOM via lit-html. Setting properties inside
     * this method will *not* trigger another update.
     * * @param _changedProperties Map of changed properties with old values
     */


    update(changedProperties) {
      super.update(changedProperties);
      const templateResult = this.render();

      if (templateResult instanceof TemplateResult) {
        this.constructor.render(templateResult, this.renderRoot, {
          scopeName: this.localName,
          eventContext: this
        });
      } // When native Shadow DOM is used but adoptedStyles are not supported,
      // insert styling after rendering to ensure adoptedStyles have highest
      // priority.


      if (this._needsShimAdoptedStyleSheets) {
        this._needsShimAdoptedStyleSheets = false;

        this.constructor._styles.forEach(s => {
          const style = document.createElement('style');
          style.textContent = s.cssText;
          this.renderRoot.appendChild(style);
        });
      }
    }
    /**
     * Invoked on each update to perform rendering tasks. This method must return
     * a lit-html TemplateResult. Setting properties inside this method will *not*
     * trigger the element to update.
     */


    render() {}

  }
  /**
   * Ensure this class is marked as `finalized` as an optimization ensuring
   * it will not needlessly try to `finalize`.
   *
   * Note this property name is a string to prevent breaking Closure JS Compiler
   * optimizations. See updating-element.ts for more information.
   */


  LitElement['finalized'] = true;
  /**
   * Render method used to render the lit-html TemplateResult to the element's
   * DOM.
   * @param {TemplateResult} Template to render.
   * @param {Element|DocumentFragment} Node into which to render.
   * @param {String} Element name.
   * @nocollapse
   */

  LitElement.render = render$1; // Copyright (c) 2013 Pieroxy <pieroxy@pieroxy.net>

  /* eslint-disable no-bitwise */

  const getMin = (arr, val) => arr.reduce((min, p) => Number(p[val]) < Number(min[val]) ? p : min, arr[0]);

  const getAvg = (arr, val) => arr.reduce((sum, p) => sum + Number(p[val]), 0) / arr.length;

  const getMax = (arr, val) => arr.reduce((max, p) => Number(p[val]) > Number(max[val]) ? p : max, arr[0]);

  const getTime = (date, extra, locale = 'en-US') => date.toLocaleString(locale, {
    hour: 'numeric',
    minute: 'numeric',
    ...extra
  });

  const getMilli = hours => hours * 60 ** 2 * 10 ** 3;

  const interpolateColor = (a, b, factor) => {
    const ah = +a.replace('#', '0x');
    const ar = ah >> 16;
    const ag = ah >> 8 & 0xff;
    const ab = ah & 0xff;
    const bh = +b.replace('#', '0x');
    const br = bh >> 16;
    const bg = bh >> 8 & 0xff;
    const bb = bh & 0xff;
    const rr = ar + factor * (br - ar);
    const rg = ag + factor * (bg - ag);
    const rb = ab + factor * (bb - ab);
    return `#${((1 << 24) + (rr << 16) + (rg << 8) + rb | 0).toString(16).slice(1)}`;
  };

  const getFirstDefinedItem = (...collection) => collection.find(item => typeof item !== 'undefined'); // eslint-disable-next-line max-len


  const compareArray = (a, b) => a.length === b.length && a.every((value, index) => value === b[index]);

  const URL_DOCS = 'https://github.com/kalkih/mini-graph-card/blob/master/README.md';
  const FONT_SIZE = 14;
  const FONT_SIZE_HEADER = 14;
  const MAX_BARS = 96;
  const ICONS = {
    humidity: 'hass:water-percent',
    illuminance: 'hass:brightness-5',
    temperature: 'hass:thermometer',
    battery: 'hass:battery',
    pressure: 'hass:gauge',
    power: 'hass:flash',
    signal_strength: 'hass:wifi',
    motion: 'hass:walk',
    door: 'hass:door-closed',
    window: 'hass:window-closed',
    presence: 'hass:account',
    light: 'hass:lightbulb'
  };
  const DEFAULT_COLORS = ['var(--accent-color)', '#3498db', '#e74c3c', '#9b59b6', '#f1c40f', '#2ecc71', '#1abc9c', '#34495e', '#e67e22', '#7f8c8d', '#27ae60', '#2980b9', '#8e44ad'];
  const UPDATE_PROPS = ['entity', 'line', 'length', 'fill', 'points', 'tooltip', 'abs', 'config'];
  const DEFAULT_SHOW = {
    name: true,
    icon: true,
    state: true,
    graph: 'line',
    labels: 'hover',
    labels_secondary: 'hover',
    extrema: false,
    legend: true,
    fill: true,
    points: 'hover'
  };
  const X = 0;
  const Y = 1;
  const V = 2;

  class Graph {
    constructor(width, height, margin, hours = 24, points = 1, aggregateFuncName = 'avg', groupBy = 'interval', smoothing = true) {
      const aggregateFuncMap = {
        avg: this._average,
        max: this._maximum,
        min: this._minimum
      };
      this.coords = [];
      this.width = width - margin[X] * 2;
      this.height = height - margin[Y] * 4;
      this.margin = margin;
      this._max = 0;
      this._min = 0;
      this.points = points;
      this.hours = hours;
      this._calcPoint = aggregateFuncMap[aggregateFuncName] || this._average;
      this._smoothing = false;
      this._groupBy = groupBy;
      this._endTime = 0;
    }

    get max() {
      return this._max;
    }

    set max(max) {
      this._max = max;
    }

    get min() {
      return this._min;
    }

    set min(min) {
      this._min = min;
    }

    update(history) {
      this._updateEndTime();

      var date_l = Date.parse(history[history.length - 1].last_changed);
      var date_f = Date.parse(history[0].last_changed);
      this.hours = (date_l - date_f) / (1000 * 60 * 60);
      const coords = history; //.reduce((res, item) => this._reducer(res, item), []);
      // extend length to fill missing history

      const requiredNumOfPoints = Math.ceil(this.hours * this.points);
      coords.length = requiredNumOfPoints; //console.log(coords);

      this.coords = this._calcPoints(coords); //console.log(this.coords);

      this.min = Math.min(...this.coords.map(item => Number(item[V])));
      this.max = Math.max(...this.coords.map(item => Number(item[V])));
    }

    _calcPoints(history) {
      const coords = [];
      var date_l = Date.parse(history[history.length - 1].last_changed);
      var date_f = Date.parse(history[0].last_changed);
      let xRatio = this.width / ((date_l - date_f + 1) / 86400000); //

      xRatio = Number.isFinite(xRatio) ? xRatio : this.width;

      for (let i = 0; i < history.length; i += 1) {
        var date_c = Date.parse(history[i].last_changed);
        var date_f = Date.parse(history[0].last_changed);
        const x = xRatio * (date_c - date_f) / 86400000;
        coords.push([x, 0, history[i].state]);
      }

      return coords;
    }

    _calcY(coords) {
      const yRatio = (this.max - this.min) / this.height || 1;
      return coords.map(coord => [coord[X], this.height - (coord[V] - this.min) / yRatio + this.margin[Y] * 2, coord[V]]);
    }

    getPoints() {
      let {
        coords
      } = this;

      if (coords.length === 1) {
        coords[1] = [this.width + this.margin[X], 0, coords[0][V]];
      }

      coords = this._calcY(this.coords);
      let next;
      let Z;
      let last = coords[0];
      coords.shift();
      const coords2 = coords.map((point, i) => {
        next = point;
        Z = this._smoothing ? this._midPoint(last[X], last[Y], next[X], next[Y]) : next;
        const sum = this._smoothing ? (next[V] + last[V]) / 2 : next[V];
        last = next;
        return [Z[X], Z[Y], sum, i + 1];
      });
      return coords2;
    }

    getPath() {
      let {
        coords
      } = this;

      if (coords.length === 1) {
        coords[1] = [this.width + this.margin[X], 0, coords[0][V]];
      }

      coords = this._calcY(this.coords);
      let next;
      let Z;
      let path = '';
      let last = coords[0];
      path += `M${last[X]},${last[Y]}`;
      coords.forEach(point => {
        next = point;
        Z = this._smoothing ? this._midPoint(last[X], last[Y], next[X], next[Y]) : next;
        path += ` ${Z[X]},${Z[Y]}`;
        path += ` Q ${next[X]},${next[Y]}`;
        last = next;
      });
      path += ` ${next[X]},${next[Y]}`;
      return path;
    }

    computeGradient(thresholds) {
      const scale = this._max - this._min;
      return thresholds.map((stop, index, arr) => {
        let color;

        if (stop.value > this._max && arr[index + 1]) {
          const factor = (this._max - arr[index + 1].value) / (stop.value - arr[index + 1].value);
          color = interpolateColor(arr[index + 1].color, stop.color, factor);
        } else if (stop.value < this._min && arr[index - 1]) {
          const factor = (arr[index - 1].value - this._min) / (arr[index - 1].value - stop.value);
          color = interpolateColor(arr[index - 1].color, stop.color, factor);
        }

        return {
          color: color || stop.color,
          offset: scale <= 0 ? 0 : (this._max - stop.value) * (100 / scale)
        };
      });
    }

    getFill(path) {
      const height = this.height + this.margin[Y] * 4;
      let fill = path;
      fill += ` L ${this.width - this.margin[X] * 2}, ${height}`;
      fill += ` L ${this.coords[0][X]}, ${height} z`;
      return fill;
    }

    getBars(position, total) {
      const coords = this._calcY(this.coords);

      const margin = 4;
      const xRatio = (this.width - margin) / Math.ceil(this.hours * this.points) / total;
      return coords.map((coord, i) => ({
        x: xRatio * i * total + xRatio * position + margin,
        y: coord[Y],
        height: this.height - coord[Y] + this.margin[Y] * 4,
        width: xRatio - margin,
        value: coord[V]
      }));
    }

    _midPoint(Ax, Ay, Bx, By) {
      const Zx = (Ax - Bx) / 2 + Bx;
      const Zy = (Ay - By) / 2 + By;
      return [Zx, Zy];
    }

    _average(items) {
      return items.reduce((sum, entry) => sum + parseFloat(entry.state), 0) / items.length;
    }

    _maximum(items) {
      return Math.max(...items.map(item => item.state));
    }

    _minimum(items) {
      return Math.min(...items.map(item => item.state));
    }

    _last(items) {
      return parseFloat(items[items.length - 1].state) || 0;
    }

    _updateEndTime() {
      this._endTime = new Date();

      switch (this._groupBy) {
        case 'date':
          this._endTime.setDate(this._endTime.getDate() + 1);

          this._endTime.setHours(0, 0);

          break;

        case 'hour':
          this._endTime.setHours(this._endTime.getHours() + 1);

          this._endTime.setMinutes(0, 0, 0);

          break;

        default:
          break;
      }
    }

  }

  const style = css`
  :host {
    display: flex;
    flex-direction: column;
  }
  ha-card {
    flex-direction: column;
    flex: 1;
    padding: 16px 0;
    position: relative;
    overflow: hidden;
  }
  ha-card > div {
    padding: 0px 16px 16px 16px;
  }
  ha-card > div:last-child {
    padding-bottom: 0;
  }
  ha-card[points] .line--points,
  ha-card[labels] .graph__labels.--primary {
    opacity: 0;
    transition: opacity .25s;
    animation: none;
  }
  ha-card[labels-secondary] .graph__labels.--secondary {
    opacity: 0;
    transition: opacity .25s;
    animation: none;
  }
  ha-card[points]:hover .line--points,
  ha-card:hover .graph__labels.--primary,
  ha-card:hover .graph__labels.--secondary {
      opacity: 1;
  }
  ha-card[fill] {
    padding-bottom: 0;
  }
  ha-card[fill] .graph {
    padding: 0;
    order: 10;
  }
  ha-card[fill] path {
    stroke-linecap: initial;
    stroke-linejoin: initial;
  }
  ha-card[fill] .graph__legend {
    order: -1;
    padding: 0 16px 8px 16px;
  }
  ha-card[fill] .info {
    padding-bottom: 16px;
  }
  ha-card[group] {
    box-shadow: none;
    padding: 0;
  }
  ha-card[group] > div {
    padding-left: 0;
    padding-right: 0;
  }
  ha-card[group] .graph__legend {
    padding-left: 0;
    padding-right: 0;
  }
  ha-card[hover] {
    cursor: pointer;
  }
  .flex {
    display: flex;
    display: -webkit-flex;
    min-width: 0;
  }
  .header {
    justify-content: space-between;
  }
  .header[loc="center"] {
    justify-content: space-around;
  }
  .header[loc="left"] {
    align-self: flex-start;
  }
  .header[loc="right"] {
    align-self: flex-end;
  }
  .name {
    align-items: center;
    min-width: 0;
    letter-spacing: var(--mcg-title-letter-spacing, normal);
  }
  .name > span {
    font-size: 1.2em;
    font-weight: var(--mcg-title-font-weight, 500);
    max-height: 1.4em;
    min-height: 1.4em;
    opacity: .65;
  }
  .icon {
    color: var(--paper-item-icon-color, #44739e);
    display: inline-block;
    flex: 0 0 1.7em;
    text-align: center;
  }
  .icon > ha-icon {
    height: 1.7em;
    width: 1.7em;
  }
  .icon[loc="left"] {
    order: -1;
    margin-right: .6em;
    margin-left: 0;
  }
  .icon[loc="state"] {
    align-self: center;
  }
  .states {
    align-items: flex-start;
    font-weight: 300;
    justify-content: space-between;
    flex-wrap: nowrap;
  }
  .states .icon {
    align-self: center;
    margin-left: 0;
  }
  .states[loc="center"] {
    justify-content: space-evenly;
  }
  .states[loc="right"] > .state {
    margin-left: auto;
    order: 2;
  }
  .states[loc="center"] .states--secondary,
  .states[loc="right"] .states--secondary {
    margin-left: 0;
  }
  .states[loc="center"] .states--secondary {
    align-items: center;
  }
  .states[loc="right"] .states--secondary {
    align-items: flex-start;
  }
  .states[loc="center"] .state__time {
    left: 50%;
    transform: translateX(-50%);
  }
  .states > .icon > ha-icon {
    height: 2em !important;
    width: 2em !important;
  }
  .states--secondary {
    display: flex;
    flex-flow: column;
    flex-wrap: wrap;
    align-items: flex-end;
    margin-left: 1rem;
    min-width: 0;
    margin-left: 1.4em;
  }
  .states--secondary:empty {
    display: none;
  }
  .state {
    position: relative;
    display: flex;
    flex-wrap: nowrap;
    max-width: 100%;
    min-width: 0;
  }
  .state--small {
    font-size: .6em;
    margin-bottom: .6rem;
    flex-wrap: nowrap;
  }
  .state--small > svg {
    position: absolute;
    left: -1.6em;
    align-self: center;
    height: 1em;
    width: 1em;
    border-radius: 100%;
    margin-right: 1em;
  }
  .state--small:last-child {
    margin-bottom: 0;
  }
  .states--secondary > :only-child {
    font-size: 1em;
    margin-bottom: 0;
  }
  .states--secondary > :only-child svg {
    display: none;
  }
  .state__value {
    display: inline-block;
    font-size: 2.4em;
    margin-right: .25rem;
    line-height: 1.2em;
  }
  .state__uom {
    flex: 1;
    align-self: flex-end;
    display: inline-block;
    font-size: 1.4em;
    font-weight: 400;
    line-height: 1.6em;
    margin-top: .1em;
    opacity: .6;
    vertical-align: bottom;
  }
  .state--small .state__uom {
    flex: 1;
  }
  .state__time {
    font-size: .95rem;
    font-weight: 500;
    bottom: -1.1rem;
    left: 0;
    opacity: .75;
    position: absolute;
    white-space: nowrap;
    animation: fade .15s cubic-bezier(0.215, 0.61, 0.355, 1);
  }
  .states[loc="right"] .state__time {
    left: initial;
    right: 0;
  }
  .graph {
    align-self: flex-end;
    box-sizing: border-box;
    display: flex;
    flex-direction: column;
    margin-top: auto;
    width: 100%;
  }
  .graph__container {
    display: flex;
    flex-direction: row;
    position: relative;
  }
  .graph__container__svg {
    cursor: default;
    flex: 1;
  }
  svg {
    overflow: hidden;
    display: block;
  }
  path {
    stroke-linecap: round;
    stroke-linejoin: round;
  }
  .fill[anim="false"] {
    animation: reveal .25s cubic-bezier(0.215, 0.61, 0.355, 1) forwards;
  }
  .fill[anim="false"][type="fade"] {
    animation: reveal-2 .25s cubic-bezier(0.215, 0.61, 0.355, 1) forwards;
  }
  .line--points[anim="false"],
  .line[anim="false"] {
    animation: pop .25s cubic-bezier(0.215, 0.61, 0.355, 1) forwards;
  }
  .line--points[inactive],
  .line--rect[inactive],
  .fill--rect[inactive] {
    opacity: 0 !important;
    animation: none !important;
    transition: all .15s !important;
  }
  .line--points[tooltip] .line--point[inactive] {
    opacity: 0;
  }
  .line--point {
    cursor: pointer;
    fill: var(--primary-background-color, white);
    stroke-width: inherit;
  }
  .line--point:hover {
    fill: var(--mcg-hover, inherit) !important;
  }
  .bars {
    animation: pop .25s cubic-bezier(0.215, 0.61, 0.355, 1);
  }
  .bars[anim] {
    animation: bars .5s cubic-bezier(0.215, 0.61, 0.355, 1);
  }
  .bar {
    transition: opacity .25s cubic-bezier(0.215, 0.61, 0.355, 1);
  }
  .bar:hover {
    opacity: .5;
    cursor: pointer;
  }
  ha-card[gradient] .line--point:hover {
    fill: var(--primary-text-color, white);
  }
  path,
  .line--points,
  .fill {
    opacity: 0;
  }
  .line--points[anim="true"][init] {
    animation: pop .5s cubic-bezier(0.215, 0.61, 0.355, 1) forwards;
  }
  .fill[anim="true"][init] {
    animation: reveal .5s cubic-bezier(0.215, 0.61, 0.355, 1) forwards;
  }
  .fill[anim="true"][init][type="fade"] {
    animation: reveal-2 .5s cubic-bezier(0.215, 0.61, 0.355, 1) forwards;
  }
  .line[anim="true"][init] {
    animation: dash 1s cubic-bezier(0.215, 0.61, 0.355, 1) forwards;
  }
  .graph__labels.--secondary {
    right: 0;
    margin-right: 0px;
  }
  .graph__labels {
    align-items: flex-start;
    flex-direction: column;
    font-size: calc(.15em + 8.5px);
    font-weight: 400;
    justify-content: space-between;
    margin-right: 10px;
    padding: .6em;
    position: absolute;
    pointer-events: none;
    top: 0; bottom: 0;
    opacity: .75;
  }
  .graph__labels > span {
    cursor: pointer;
    background: var(--primary-background-color, white);
    border-radius: 1em;
    padding: .2em .6em;
    box-shadow: 0 1px 3px rgba(0,0,0,.12), 0 1px 2px rgba(0,0,0,.24);
  }
  .graph__legend {
    display: flex;
    flex-direction: row;
    justify-content: space-evenly;
    padding-top: 16px;
    flex-wrap: wrap;
  }
  .graph__legend__item {
    cursor: pointer;
    display: flex;
    min-width: 0;
    margin: .4em;
    align-items: center
  }
  .graph__legend__item span {
    opacity: .75;
    margin-left: .4em;
  }
  .graph__legend__item svg {
    border-radius: 100%;
    min-width: 10px;
  }
  .info {
    justify-content: space-between;
    align-items: middle;
  }
  .info__item {
    display: flex;
    flex-flow: column;
    text-align: center;
  }
  .info__item:first-child {
    align-items: flex-start;
    text-align: left;
  }
  .info__item:last-child {
    align-items: flex-end;
    text-align: right;
  }
  .info__item__type {
    text-transform: capitalize;
    font-weight: 500;
    opacity: .9;
  }
  .info__item__time,
  .info__item__value {
    opacity: .75;
  }
  .ellipsis {
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }
  @keyframes fade {
    0% { opacity: 0; }
  }
  @keyframes reveal {
    0% { opacity: 0; }
    100% { opacity: .15; }
  }
  @keyframes reveal-2 {
    0% { opacity: 0; }
    100% { opacity: .4; }
  }
  @keyframes pop {
    0% { opacity: 0; }
    100% { opacity: 1; }
  }
  @keyframes bars {
    0% { opacity: 0; }
    50% { opacity: 0; }
    100% { opacity: 1; }
  }
  @keyframes dash {
    0% {
      opacity: 0;
    }
    25% {
      opacity: 1;
    }
    100% {
      opacity: 1;
      stroke-dashoffset: 0;
    }
  }`;

  var handleClick = (node, hass, config, actionConfig, entityId) => {
    let e; // eslint-disable-next-line default-case

    switch (actionConfig.action) {
      case 'more-info':
        {
          e = new Event('hass-more-info', {
            composed: true
          });
          e.detail = {
            entityId
          };
          node.dispatchEvent(e);
          break;
        }

      case 'navigate':
        {
          if (!actionConfig.navigation_path) return;
          window.history.pushState(null, '', actionConfig.navigation_path);
          e = new Event('location-changed', {
            composed: true
          });
          e.detail = {
            replace: false
          };
          window.dispatchEvent(e);
          break;
        }

      case 'call-service':
        {
          if (!actionConfig.service) return;
          const [domain, service] = actionConfig.service.split('.', 2);
          const serviceData = { ...actionConfig.service_data
          };
          hass.callService(domain, service, serviceData);
          break;
        }

      case 'url':
        {
          if (!actionConfig.url) return;
          window.location.href = actionConfig.url;
        }
    }
  };

  function parseDate(eventDate, formater) {
    const normalizedFormat = formater.toLowerCase().replace(/[^a-zA-Z0-9]/g, '-');
    const formatItems = normalizedFormat.split('-');
    const monthIndex = formatItems.indexOf("mm");
    const dayIndex = formatItems.indexOf("dd");
    const yearIndex = formatItems.indexOf("yyyy");
    const hourIndex = formatItems.indexOf("hh");
    const minutesIndex = formatItems.indexOf("ii");
    const secondsIndex = formatItems.indexOf("ss");
    const today = new Date(); // variable format date parseing

    var normalized = eventDate.replace(/[^a-zA-Z0-9]/g, '-');
    var dateItems = normalized.split('-');
    var year = yearIndex > -1 ? dateItems[yearIndex] : today.getFullYear();
    var month = monthIndex > -1 ? dateItems[monthIndex] - 1 : today.getMonth() - 1;
    var day = dayIndex > -1 ? dateItems[dayIndex] : today.getDate();
    var hour = hourIndex > -1 ? dateItems[hourIndex] : 0;
    var minute = minutesIndex > -1 ? dateItems[minutesIndex] : 0;
    var second = secondsIndex > -1 ? dateItems[secondsIndex] : 0;
    return new Date(year, month, day, hour, minute, second); // variable format date parseing
  }

  class MiniGraphCard extends LitElement {
    constructor() {
      super();
      this.id = Math.random().toString(36).substr(2, 9);
      this.config = {};
      this.bound = [0, 0];
      this.boundSecondary = [0, 0];
      this.min = {};
      this.avg = {};
      this.max = {};
      this.length = [];
      this.entity = [];
      this.line = [];
      this.bar = [];
      this.data = [];
      this.fill = [];
      this.points = [];
      this.gradient = [];
      this.display_mode = '';
      this.data_delimiter = '';
      this.tooltip = {};
      this.updateQueue = [];
      this.updating = false;
      this.stateChanged = false;
    }

    static get styles() {
      return style;
    }

    set hass(hass) {
      this._hass = hass;
      let updated = false;
      this.config.entities.forEach((entity, index) => {
        this.config.entities[index].index = index; // Required for filtered views

        const entityState = hass.states[entity.entity];

        if (entityState && this.entity[index] !== entityState) {
          this.entity[index] = entityState;
          this.updateQueue.push(entityState.entity_id);
          updated = true;
        }
      });

      if (updated) {
        this.entity = [...this.entity];

        if (!this.config.update_interval && !this.updating) {
          this.updateData();
        } else {
          this.stateChanged = true;
        }
      }
    }

    static get properties() {
      return {
        id: String,
        _hass: {},
        config: {},
        entity: [],
        Graph: [],
        line: [],
        shadow: [],
        length: Number,
        bound: [],
        boundSecondary: [],
        abs: [],
        tooltip: {},
        updateQueue: [],
        color: String
      };
    }

    setConfig(config) {
      if (config.entity) throw new Error(`The "entity" option was removed, please use "entities".\n See ${URL_DOCS}`);
      if (!Array.isArray(config.entities)) throw new Error(`Please provide the "entities" option as a list.\n See ${URL_DOCS}`);
      if (config.line_color_above || config.line_color_below) throw new Error(`"line_color_above/line_color_below" was removed, please use "color_thresholds".\n See ${URL_DOCS}`);
      const conf = {
        animate: false,
        hour24: false,
        font_size: FONT_SIZE,
        font_size_header: FONT_SIZE_HEADER,
        height: 100,
        hours_to_show: 24 * 7,
        points_per_hour: 1 / 24,
        aggregate_func: 'avg',
        group_by: 'date',
        line_color: [...DEFAULT_COLORS],
        color_thresholds: [],
        color_thresholds_transition: 'smooth',
        line_width: 5,
        compress: false,
        smoothing: false,
        unit: '',
        maxDays: 30,
        formater: 'dd.mm.yyyy',
        display_mode: 'abs',
        data_delimiter: ';',
        state_map: [],
        tap_action: {
          action: 'more-info'
        },
        ...config,
        show: { ...DEFAULT_SHOW,
          ...config.show
        }
      };
      conf.entities.forEach((entity, i) => {
        if (typeof entity === 'string') conf.entities[i] = {
          entity
        };
      });
      conf.state_map.forEach((state, i) => {
        // convert string values to objects
        if (typeof state === 'string') conf.state_map[i] = {
          value: state,
          label: state
        }; // make sure label is set

        conf.state_map[i].label = conf.state_map[i].label || conf.state_map[i].value;
      });
      if (typeof config.line_color === 'string') conf.line_color = [config.line_color, ...DEFAULT_COLORS];
      conf.font_size = config.font_size / 100 * FONT_SIZE || FONT_SIZE;
      conf.color_thresholds = this.computeThresholds(conf.color_thresholds, conf.color_thresholds_transition);
      const additional = conf.hours_to_show > 24 ? {
        day: 'numeric',
        weekday: 'short'
      } : {};
      conf.format = {
        hour12: !conf.hour24,
        ...additional
      }; // override points per hour to mach group_by function

      switch (conf.group_by) {
        case 'date':
          conf.points_per_hour = 1 / 24;
          break;

        case 'hour':
          conf.points_per_hour = 1;
          break;

        default:
          break;
      }

      if (conf.show.graph === 'bar') {
        const entities = conf.entities.length;

        if (conf.hours_to_show * conf.points_per_hour * entities > MAX_BARS) {
          conf.points_per_hour = MAX_BARS / (conf.hours_to_show * entities); // eslint-disable-next-line no-console

          console.warn('mini-graph-card: Not enough space, adjusting points_per_hour to ', conf.points_per_hour);
        }
      }

      const entitiesChanged = !compareArray(this.config.entities || [], conf.entities);
      this.config = conf;

      if (!this.Graph || entitiesChanged) {
        if (this._hass) this.hass = this._hass;
        this.Graph = conf.entities.map(entity => new Graph(500, conf.height, [conf.show.fill ? 0 : conf.line_width, conf.line_width], conf.hours_to_show, conf.points_per_hour, entity.aggregate_func || conf.aggregate_func, conf.group_by, getFirstDefinedItem(entity.smoothing, config.smoothing, !entity.entity.startsWith('binary_sensor.') // turn off for binary sensor by default
        )));
      }
    }

    connectedCallback() {
      super.connectedCallback();

      if (this.config.update_interval) {
        this.updateOnInterval();
        this.interval = setInterval(() => this.updateOnInterval(), this.config.update_interval * 1000);
      }
    }

    disconnectedCallback() {
      if (this.interval) {
        clearInterval(this.interval);
      }

      super.disconnectedCallback();
    }

    shouldUpdate(changedProps) {
      if (!this.entity[0]) return false;

      if (UPDATE_PROPS.some(prop => changedProps.has(prop))) {
        this.color = this.intColor(this.tooltip.value !== undefined ? this.tooltip.value : this.entity[0].state, this.tooltip.entity || 0);
        return true;
      }
    }

    updated(changedProperties) {
      if (this.config.animate && changedProperties.has('line')) {
        if (this.length.length < this.entity.length) {
          this.shadowRoot.querySelectorAll('svg path.line').forEach(ele => {
            this.length[ele.id] = ele.getTotalLength();
          });
          this.length = [...this.length];
        } else {
          this.length = Array(this.entity.length).fill('none');
        }
      }
    }

    render({
      config
    } = this) {
      return html`
      <ha-card
        class="flex"
        ?group=${config.group}
        ?fill=${config.show.graph && config.show.fill}
        ?points=${config.show.points === 'hover'}
        ?labels=${config.show.labels === 'hover'}
        ?labels-secondary=${config.show.labels_secondary === 'hover'}
        ?gradient=${config.color_thresholds.length > 0}
        ?hover=${config.tap_action.action !== 'none'}
        style="font-size: ${config.font_size}px;"
        @click=${e => this.handlePopup(e, this.entity[0])}
      >
        ${this.renderHeader()} ${this.renderStates()} ${this.renderGraph()} ${this.renderInfo()}
      </ha-card>
    `;
    }

    renderHeader() {
      const {
        show,
        align_icon,
        align_header,
        font_size_header
      } = this.config;
      return show.name || show.icon && align_icon !== 'state' ? html`
          <div class="header flex" loc=${align_header} style="font-size: ${font_size_header}px;">
            ${this.renderName()} ${align_icon !== 'state' ? this.renderIcon() : ''}
          </div>
        ` : '';
    }

    renderIcon() {
      const {
        icon,
        icon_adaptive_color
      } = this.config.show;
      return icon ? html`
      <div class="icon" loc=${this.config.align_icon}
        style=${icon_adaptive_color ? `color: ${this.color};` : ''}>
        <ha-icon .icon=${this.computeIcon(this.entity[0])}></ha-icon>
      </div>
    ` : '';
    }

    renderName() {
      if (!this.config.show.name) return;
      const name = this.config.name || this.computeName(0);
      const color = this.config.show.name_adaptive_color ? `opacity: 1; color: ${this.color};` : '';
      return html`
      <div class="name flex">
        <span class="ellipsis" style=${color}>${name}</span>
      </div>
    `;
    }

    renderStates() {
      const {
        entity,
        value
      } = this.tooltip;
      const state = value !== undefined ? value : this.entity[0].state;
      const color = this.config.entities[0].state_adaptive_color ? `color: ${this.color};` : '';
      if (this.config.show.state) return html`
        <div class="states flex" loc=${this.config.align_state}>
          <div class="state">
            <span class="state__value ellipsis" style=${color}>
              ${this.computeState(state)}
            </span>
            <span class="state__uom ellipsis" style=${color}>
              ${this.computeUom(entity || 0)}
            </span>
            ${this.renderStateTime()}
          </div>
          <div class="states--secondary">${this.config.entities.map((ent, i) => this.renderState(ent, i))}</div>
          ${this.config.align_icon === 'state' ? this.renderIcon() : ''}
        </div>
      `;
    }

    renderState(entity, id) {
      if (entity.show_state && id !== 0) {
        const {
          state
        } = this.entity[id];
        return html`
        <div
          class="state state--small"
          @click=${e => this.handlePopup(e, this.entity[id])}
          style=${entity.state_adaptive_color ? `color: ${this.computeColor(state, id)};` : ''}>
          ${entity.show_indicator ? this.renderIndicator(state, id) : ''}
          <span class="state__value ellipsis">
            ${this.computeState(state)}
          </span>
          <span class="state__uom ellipsis">
            ${this.computeUom(id)}
          </span>
        </div>
      `;
      }
    }

    renderStateTime() {
      if (this.tooltip.value === undefined) return;
      return html`
      <div class="state__time">
        ${this.tooltip.label ? html`
          <span>${this.tooltip.label}</span>
        ` : html`
          <span>${this.tooltip.time[0]}</span>
        `}
      </div>
    `;
    }

    renderGraph() {
      return this.config.show.graph ? html`
      <div class="graph">
        <div class="graph__container">
          ${this.renderLabels()}
          ${this.renderLabelsSecondary()}
          <div class="graph__container__svg">
            ${this.renderSvg()}
          </div>
        </div>
        ${this.renderLegend()}
      </div>` : '';
    }

    renderLegend() {
      if (this.visibleLegends.length <= 1 || !this.config.show.legend) return;
      return html`
      <div class="graph__legend">
        ${this.visibleLegends.map(entity => html`
          <div class="graph__legend__item"
            @click=${e => this.handlePopup(e, this.entity[entity.index])}
            @mouseover=${() => this.setTooltip(entity.index, -1, this.entity[entity.index].state, 'Current')}
            >
            ${this.renderIndicator(this.entity[entity.index].state, entity.index)}
            <span class="ellipsis">${this.computeName(entity.index)}</span>
          </div>
        `)}
      </div>
    `;
    }

    renderIndicator(state, index) {
      return svg`
      <svg width='10' height='10'>
        <rect width='10' height='10' fill=${this.intColor(state, index)} />
      </svg>
    `;
    }

    renderSvgFill(fill, i) {
      if (!fill) return;
      const fade = this.config.show.fill === 'fade';
      return svg`
      <defs>
        <linearGradient id=${`fill-grad-${this.id}-${i}`} x1="0%" y1="0%" x2="0%" y2="100%">
          <stop stop-color='white' offset='0%' stop-opacity='1'/>
          <stop stop-color='white' offset='100%' stop-opacity='.15'/>
        </linearGradient>
        <mask id=${`fill-grad-mask-${this.id}-${i}`}>
          <rect width="100%" height="100%" fill=${`url(#fill-grad-${this.id}-${i})`} />
        </mask>
      </defs>
      <mask id=${`fill-${this.id}-${i}`}>
        <path class='fill'
          type=${this.config.show.fill}
          .id=${i} anim=${this.config.animate} ?init=${this.length[i]}
          style="animation-delay: ${this.config.animate ? `${i * 0.5}s` : '0s'}"
          fill='white'
          mask=${fade ? `url(#fill-grad-mask-${this.id}-${i})` : ''}
          d=${this.fill[i]}
        />
      </mask>`;
    }

    renderSvgLine(line, i) {
      if (!line) return;
      const path = svg`
      <path
        class='line'
        .id=${i}
        anim=${this.config.animate} ?init=${this.length[i]}
        style="animation-delay: ${this.config.animate ? `${i * 0.5}s` : '0s'}"
        fill='none'
        stroke-dasharray=${this.length[i] || 'none'} stroke-dashoffset=${this.length[i] || 'none'}
        stroke=${'white'}
        stroke-width=${this.config.line_width}
        d=${this.line[i]}
      />`;
      return svg`
      <mask id=${`line-${this.id}-${i}`}>
        ${path}
      </mask>
    `;
    }

    renderSvgPoint(point, i) {
      const color = this.gradient[i] ? this.computeColor(point[V], i) : 'inherit';
      return svg`
      <circle
        class='line--point'
        ?inactive=${this.tooltip.index !== point[3]}
        style=${`--mcg-hover: ${color};`}
        stroke=${color}
        fill=${color}
        cx=${point[X]} cy=${point[Y]} r=${this.config.line_width}
        @mouseover=${() => this.setTooltip(i, point[3], point[V])}

      />
    `;
    }

    renderSvgPoints(points, i) {
      if (!points) return;
      const color = this.computeColor(this.entity[i].state, i);
      return svg`
      <g class='line--points'
        ?tooltip=${this.tooltip.entity === i}
        ?inactive=${this.tooltip.entity !== undefined && this.tooltip.entity !== i}
        ?init=${this.length[i]}
        anim=${this.config.animate && this.config.show.points !== 'hover'}
        style="animation-delay: ${this.config.animate ? `${i * 0.5 + 0.5}s` : '0s'}"
        fill=${color}
        stroke=${color}
        stroke-width=${this.config.line_width / 2}>
        ${points.map(point => this.renderSvgPoint(point, i))}
      </g>`;
    }

    renderSvgGradient(gradients) {
      if (!gradients) return;
      const items = gradients.map((gradient, i) => {
        if (!gradient) return;
        return svg`
        <linearGradient id=${`grad-${this.id}-${i}`} gradientTransform="rotate(90)">
          ${gradient.map(stop => svg`
            <stop stop-color=${stop.color} offset=${`${stop.offset}%`} />
          `)}
        </linearGradient>`;
      });
      return svg`${items}`;
    }

    renderSvgLineRect(line, i) {
      if (!line) return;
      const fill = this.gradient[i] ? `url(#grad-${this.id}-${i})` : this.computeColor(this.entity[i].state, i);
      return svg`
      <rect class='line--rect'
        ?inactive=${this.tooltip.entity !== undefined && this.tooltip.entity !== i}
        id=${`rect-${this.id}-${i}`}
        fill=${fill} height="100%" width="100%"
        mask=${`url(#line-${this.id}-${i})`}
      />`;
    }

    renderSvgFillRect(fill, i) {
      if (!fill) return;
      const svgFill = this.gradient[i] ? `url(#grad-${this.id}-${i})` : this.intColor(this.entity[i].state, i);
      return svg`
      <rect class='fill--rect'
        ?inactive=${this.tooltip.entity !== undefined && this.tooltip.entity !== i}
        id=${`fill-rect-${this.id}-${i}`}
        fill=${svgFill} height="100%" width="100%"
        mask=${`url(#fill-${this.id}-${i})`}
      />`;
    }

    renderSvgBars(bars, index) {
      if (!bars) return;
      const items = bars.map((bar, i) => {
        const animation = this.config.animate ? svg`
          <animate attributeName='y' from=${this.config.height} to=${bar.y} dur='1s' fill='remove'
            calcMode='spline' keyTimes='0; 1' keySplines='0.215 0.61 0.355 1'>
          </animate>` : '';
        const color = this.computeColor(bar.value, index);
        return svg`
        <rect class='bar' x=${bar.x} y=${bar.y}
          height=${bar.height} width=${bar.width} fill=${color}
          @mouseover=${() => this.setTooltip(index, i, bar.value)}
          >
          ${animation}
        </rect>`;
      });
      return svg`<g class='bars' ?anim=${this.config.animate}>${items}</g>`;
    }

    renderSvg() {
      const {
        height
      } = this.config;
      return svg`
      <svg width='100%' height=${height !== 0 ? '100%' : 0} viewBox='0 0 500 ${height}'
        @click=${e => e.stopPropagation()}>
        <g>
          <defs>
            ${this.renderSvgGradient(this.gradient)}
          </defs>
          ${this.fill.map((fill, i) => this.renderSvgFill(fill, i))}
          ${this.fill.map((fill, i) => this.renderSvgFillRect(fill, i))}
          ${this.line.map((line, i) => this.renderSvgLine(line, i))}
          ${this.line.map((line, i) => this.renderSvgLineRect(line, i))}
          ${this.bar.map((bars, i) => this.renderSvgBars(bars, i))}
        </g>
        ${this.points.map((points, i) => this.renderSvgPoints(points, i))}
      </svg>`;
    }

    setTooltip(entity, index, value, label = null) {
      const {
        points_per_hour,
        hours_to_show,
        format
      } = this.config;
      const offset = hours_to_show < 1 && points_per_hour < 1 ? points_per_hour * hours_to_show : 1 / points_per_hour;
      const id = Math.abs(index + 1 - Math.ceil(hours_to_show * points_per_hour));
      const now = this.getEndDate();
      const oneMinInHours = 1 / 60;
      now.setMilliseconds(now.getMilliseconds() - getMilli(offset * id + oneMinInHours));
      const end = getTime(now, {
        hour12: !this.config.hour24
      }, this._hass.language);
      var start = end; //

      if (this.data.length > index) {
        start = this.data[index].last_changed_org;
      }

      this.tooltip = {
        value,
        id,
        entity,
        time: [start, end],
        index,
        label
      };
    }

    renderLabels() {
      if (!this.config.show.labels || this.primaryYaxisSeries.length === 0) return;
      return html`
      <div class="graph__labels --primary flex">
        <span class="label--max">${this.computeState(this.bound[1])}</span>
        <span class="label--min">${this.computeState(this.bound[0])}</span>
      </div>
    `;
    }

    renderLabelsSecondary() {
      if (!this.config.show.labels_secondary || this.secondaryYaxisSeries.length === 0) return;
      return html`
      <div class="graph__labels --secondary flex">
        <span class="label--max">${this.computeState(this.boundSecondary[1])}</span>
        <span class="label--min">${this.computeState(this.boundSecondary[0])}</span>
      </div>
    `;
    }

    renderInfo() {
      const info = [];
      if (this.config.show.extrema) info.push(this.min);
      if (this.config.show.average) info.push(this.avg);
      if (this.config.show.extrema) info.push(this.max);
      if (!info.length) return;
      return html`
      <div class="info flex">
        ${info.map(entry => html`
          <div class="info__item">
            <span class="info__item__type">${entry.type}</span>
            <span class="info__item__value">
              ${this.computeState(entry.state)} ${this.computeUom(0)}
            </span>
            <span class="info__item__time">
              ${entry.type !== 'avg' ? getTime(new Date(entry.last_changed), this.config.format, this._hass.language) : ''}
            </span>
          </div>
        `)}
      </div>
    `;
    }

    handlePopup(e, entity) {
      e.stopPropagation();
      handleClick(this, this._hass, this.config, this.config.tap_action, entity.entity_id);
    }

    computeThresholds(stops, type) {
      stops.sort((a, b) => b.value - a.value);

      if (type === 'smooth') {
        return stops;
      } else {
        const rect = [].concat(...stops.map((stop, i) => [stop, {
          value: stop.value - 0.0001,
          color: stops[i + 1] ? stops[i + 1].color : stop.color
        }]));
        return rect;
      }
    }

    computeColor(inState, i) {
      const {
        color_thresholds,
        line_color
      } = this.config;
      const state = Number(inState) || 0;
      const threshold = {
        color: line_color[i] || line_color[0],
        ...color_thresholds.slice(-1)[0],
        ...color_thresholds.find(ele => ele.value < state)
      };
      return this.config.entities[i].color || threshold.color;
    }

    get visibleEntities() {
      return this.config.entities.filter(entity => entity.show_graph !== false);
    }

    get primaryYaxisEntities() {
      return this.visibleEntities.filter(entity => entity.y_axis === undefined || entity.y_axis === 'primary');
    }

    get secondaryYaxisEntities() {
      return this.visibleEntities.filter(entity => entity.y_axis === 'secondary');
    }

    get visibleLegends() {
      return this.visibleEntities.filter(entity => entity.show_legend !== false);
    }

    get primaryYaxisSeries() {
      return this.primaryYaxisEntities.map(entity => this.Graph[entity.index]);
    }

    get secondaryYaxisSeries() {
      return this.secondaryYaxisEntities.map(entity => this.Graph[entity.index]);
    }

    intColor(inState, i) {
      const {
        color_thresholds,
        line_color
      } = this.config;
      const state = Number(inState) || 0;
      let intColor;

      if (color_thresholds.length > 0) {
        if (this.config.show.graph === 'bar') {
          const {
            color
          } = color_thresholds.find(ele => ele.value < state) || color_thresholds.slice(-1)[0];
          intColor = color;
        } else {
          const index = color_thresholds.findIndex(ele => ele.value < state);
          const c1 = color_thresholds[index];
          const c2 = color_thresholds[index - 1];

          if (c2) {
            const factor = (c2.value - inState) / (c2.value - c1.value);
            intColor = interpolateColor(c2.color, c1.color, factor);
          } else {
            intColor = index ? color_thresholds[color_thresholds.length - 1].color : color_thresholds[0].color;
          }
        }
      }

      return this.config.entities[i].color || intColor || line_color[i] || line_color[0];
    }

    computeName(index) {
      return this.config.entities[index].name || this.entity[index].attributes.friendly_name;
    }

    computeIcon(entity) {
      return this.config.icon || entity.attributes.icon || ICONS[entity.attributes.device_class] || ICONS.temperature;
    }

    computeUom(index) {
      return this.config.entities[index].unit || this.config.unit || this.entity[index].attributes.unit_of_measurement || '';
    }

    computeState(inState) {
      if (this.config.state_map.length > 0) {
        const stateMap = Number.isInteger(inState) ? this.config.state_map[inState] : this.config.state_map.find(state => state.value === inState);

        if (stateMap) {
          return stateMap.label;
        } else {
          // eslint-disable-next-line no-console
          console.warn(`mini-graph-card: value [${inState}] not found in state_map`);
        }
      }

      let state;

      if (typeof inState === 'string') {
        state = parseFloat(inState.replace(/,/g, '.'));
      } else {
        state = Number(inState);
      }

      const dec = this.config.decimals;
      if (dec === undefined || Number.isNaN(dec) || Number.isNaN(state)) return Math.round(state * 100) / 100;
      const x = 10 ** dec;
      return (Math.round(state * x) / x).toFixed(dec);
    }

    updateOnInterval() {
      if (this.stateChanged && !this.updating) {
        this.stateChanged = false;
        this.updateData();
      }
    }

    async updateData({
      config
    } = this) {
      this.updating = true;
      const end = this.getEndDate();
      const start = new Date();
      start.setHours(end.getHours() - config.hours_to_show);

      try {
        const promise = this.entity.map((entity, i) => this.updateEntity(entity, i, start, end));
        await Promise.all(promise);
      } finally {
        this.updating = false;
      }

      this.updateQueue = [];
      this.bound = [config.lower_bound !== undefined ? config.lower_bound : Math.min(...this.primaryYaxisSeries.map(ele => ele.min)) || this.bound[0], config.upper_bound !== undefined ? config.upper_bound : Math.max(...this.primaryYaxisSeries.map(ele => ele.max)) || this.bound[1]];
      this.boundSecondary = [config.lower_bound_secondary !== undefined ? config.lower_bound_secondary : Math.min(...this.secondaryYaxisSeries.map(ele => ele.min)) || this.boundSecondary[0], config.upper_bound_secondary !== undefined ? config.upper_bound_secondary : Math.max(...this.secondaryYaxisSeries.map(ele => ele.max)) || this.boundSecondary[1]];

      if (config.show.graph) {
        this.entity.forEach((entity, i) => {
          if (!entity || this.Graph[i].coords.length === 0) return;
          const bound = config.entities[i].y_axis === 'secondary' ? this.boundSecondary : this.bound;
          [this.Graph[i].min, this.Graph[i].max] = [bound[0], bound[1]];

          if (config.show.graph === 'bar') {
            this.bar[i] = this.Graph[i].getBars(i, this.visibleEntities.length);
          } else {
            const line = this.Graph[i].getPath();
            if (config.entities[i].show_line !== false) this.line[i] = line;
            if (config.show.fill && config.entities[i].show_fill !== false) this.fill[i] = this.Graph[i].getFill(line);

            if (config.show.points && config.entities[i].show_points !== false) {
              this.points[i] = this.Graph[i].getPoints();
            }

            if (config.color_thresholds.length > 0 && !config.entities[i].color) this.gradient[i] = this.Graph[i].computeGradient(config.color_thresholds);
          }
        });
        this.line = [...this.line];
      }
    }

    async updateEntity(entity, index, initStart, end) {
      if (!entity || !this.updateQueue.includes(entity.entity_id) || this.config.entities[index].show_graph === false) return;
      let raw_data = entity.attributes.entries; // entries stores the file content

      let i_out = 0;
      let last_point = -1; // is this really the way to work around parseDate
      // clear exising data

      this.data = [];

      for (var i = 0; i < raw_data.length; i += 1) {
        if (raw_data[i].split(this.config.data_delimiter).length > 1) {
          // skip e.g. blank lines
          if (last_point == -1) {
            last_point = i;

            if (this.config.display_mode == 'diff') {
              // skip first line, can't diff it againt anything
              continue;
            }
          }

          var eventDate = raw_data[i].split(this.config.data_delimiter)[0];
          var parsedDate = parseDate(eventDate, this.config.formater); // check if data within limit

          if (Date.now() - parsedDate < this.config.maxDays * 86400000) {
            this.data[i_out] = [];
            this.data[i_out]["last_changed_org"] = eventDate;
            this.data[i_out]["last_changed"] = parsedDate.toString();

            if (this.config.display_mode == 'diff') {
              // difference in days between the data, mostly this should be 1.0
              let d_x = (parsedDate - parseDate(raw_data[last_point].split(this.config.data_delimiter)[0], this.config.formater)) / 86400000;
              let d_y = parseFloat(raw_data[i].split(this.config.data_delimiter)[1]) - parseFloat(raw_data[last_point].split(this.config.data_delimiter)[1]); // scale to 'per day'

              this.data[i_out]["state"] = d_y / d_x; // todo
            } else {
              this.data[i_out]["state"] = parseFloat(raw_data[i].split(this.config.data_delimiter)[1]);
            }

            i_out++;
          }

          last_point = i;
        }
      }

      this.setTooltip(0, i_out - 1, this.data[i_out - 1]["state"]);

      if (entity.entity_id === this.entity[0].entity_id) {
        this.min = {
          type: 'min',
          ...getMin(this.data, 'state')
        };
        this.avg = {
          type: 'avg',
          state: getAvg(this.data, 'state')
        };
        this.max = {
          type: 'max',
          ...getMax(this.data, 'state')
        };
      }

      if (this.config.entities[index].fixed_value === true) {
        const last = this.data[this.data.length - 1];
        this.Graph[index].update([last, last]);
      } else {
        this.Graph[index].update(this.data);
      }
    }

    async fetchRecent(entityId, start, end, skipInitialState) {
      let url = 'history/period';
      if (start) url += `/${start.toISOString()}`;
      url += `?filter_entity_id=${entityId}`;
      if (end) url += `&end_time=${end.toISOString()}`;
      if (skipInitialState) url += '&skip_initial_state';
      return this._hass.callApi('GET', url);
    }

    _convertState(res) {
      const resultIndex = this.config.state_map.findIndex(s => s.value === res.state);

      if (resultIndex === -1) {
        return;
      }

      res.state = resultIndex;
    }

    getEndDate() {
      const date = new Date();

      switch (this.config.group_by) {
        case 'date':
          date.setDate(date.getDate() + 1);
          date.setHours(0, 0);
          break;

        case 'hour':
          date.setHours(date.getHours() + 1);
          date.setMinutes(0, 0);
          break;

        default:
          break;
      }

      return date;
    }

    getCardSize() {
      return 3;
    }

  }

  customElements.define('mini-graph-card', MiniGraphCard);
});
