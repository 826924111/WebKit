/*
 * Copyright (C) 2005, 2006 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#ifdef KHTML_XSLT

#include "XSLTProcessor.h"
#include "xslt_processorimpl.h"
#include "XSLTProcessor.lut.h"
#include "kjs_dom.h"
#include "DocumentImpl.h"
#include "DocumentFragmentImpl.h"

using namespace WebCore;

namespace KJS {

const ClassInfo XSLTProcessor::info = { "XSLTProcessor", 0, 0, 0 };

/*
@begin XSLTProcessorProtoTable 7
  importStylesheet      XSLTProcessor::ImportStylesheet     DontDelete|Function 1
  transformToFragment   XSLTProcessor::TransformToFragment  DontDelete|Function 2
  transformToDocument   XSLTProcessor::TransformToDocument  DontDelete|Function 2
  setParameter          XSLTProcessor::SetParameter         DontDelete|Function 3
  getParameter          XSLTProcessor::GetParameter         DontDelete|Function 2
  removeParameter       XSLTProcessor::RemoveParameter      DontDelete|Function 2
  clearParameters       XSLTProcessor::ClearParameters      DontDelete|Function 0
  reset                 XSLTProcessor::Reset                DontDelete|Function 0
@end
*/

KJS_DEFINE_PROTOTYPE(XSLTProcessorProto)
KJS_IMPLEMENT_PROTOFUNC(XSLTProcessorProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("XSLTProcessor", XSLTProcessorProto, XSLTProcessorProtoFunc)

XSLTProcessor::XSLTProcessor(ExecState *exec) : m_impl(new XSLTProcessorImpl())
{
    setPrototype(XSLTProcessorProto::self(exec));
}

XSLTProcessor::~XSLTProcessor()
{
    ScriptInterpreter::forgetDOMObject(m_impl.get());
}

JSValue *XSLTProcessorProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
    if (!thisObj->inherits(&KJS::XSLTProcessor::info))
        return throwError(exec, TypeError);
    XSLTProcessorImpl &processor = *static_cast<XSLTProcessor *>(thisObj)->impl();
    switch (id) {
        case XSLTProcessor::ImportStylesheet:
        {
            JSValue *nodeVal = args[0];
            if (nodeVal->isObject(&DOMNode::info)) {
                DOMNode *node = static_cast<DOMNode *>(nodeVal);
                processor.importStylesheet(node->impl());
                return jsUndefined();
            }
            // Throw exception?
            break;
        }
        case XSLTProcessor::TransformToFragment:
        {
            JSValue *nodeVal = args[0];
            JSValue *docVal = args[1];
            if (nodeVal->isObject(&DOMNode::info) && docVal->isObject(&DOMDocument::info)) {
                NodeImpl* node = static_cast<DOMNode *>(nodeVal)->impl();
                DocumentImpl* doc = static_cast<DocumentImpl*>(static_cast<DOMDocument *>(docVal)->impl());
                return toJS(exec, processor.transformToFragment(node, doc).get());
            }
            // Throw exception?
            break;
        }
        case XSLTProcessor::TransformToDocument:
        {
            JSValue *nodeVal = args[0];
            if (nodeVal->isObject(&DOMNode::info)) {
                DOMNode *node = static_cast<DOMNode *>(nodeVal);
                RefPtr<DocumentImpl> resultDocument = processor.transformToDocument(node->impl());
                if (resultDocument)
                    return toJS(exec, resultDocument.get());
                return jsUndefined();
            }
            // Throw exception?
            break;
        }
        case XSLTProcessor::SetParameter:
        {
            if (args[1]->isUndefinedOrNull() || args[2]->isUndefinedOrNull())
                return jsUndefined(); // Throw exception?
            DOMString namespaceURI = args[0]->toString(exec).domString();
            DOMString localName = args[1]->toString(exec).domString();
            DOMString value = args[2]->toString(exec).domString();
            processor.setParameter(namespaceURI.impl(), localName.impl(), value.impl());
            return jsUndefined();
        }
        case XSLTProcessor::GetParameter:
        {
            if (args[1]->isUndefinedOrNull())
                return jsUndefined();
            DOMString namespaceURI = args[0]->toString(exec).domString();
            DOMString localName = args[1]->toString(exec).domString();
            DOMStringImpl *value = processor.getParameter(namespaceURI.impl(), localName.impl()).get();
            if (value)
                return jsString(UString(DOMString(value)));
            return jsUndefined();
        }
        case XSLTProcessor::RemoveParameter:
        {
            if (args[1]->isUndefinedOrNull())
                return jsUndefined();
            DOMString namespaceURI = args[0]->toString(exec).domString();
            DOMString localName = args[1]->toString(exec).domString();
            processor.removeParameter(namespaceURI.impl(), localName.impl());
            return jsUndefined();
        }
        case XSLTProcessor::ClearParameters:
            processor.clearParameters();
            return jsUndefined();
        case XSLTProcessor::Reset:
            processor.reset();
            return jsUndefined();
    }
    return jsUndefined();
}

XSLTProcessorConstructorImp::XSLTProcessorConstructorImp(ExecState *exec)
{
    putDirect(prototypePropertyName, XSLTProcessorProto::self(exec), DontEnum|DontDelete|ReadOnly);
}

}

#endif // KHTML_XSLT
