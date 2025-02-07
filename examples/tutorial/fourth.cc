/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/object.h"
#include "ns3/simulator.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/traced-value.h"
#include "ns3/uinteger.h"

#include <iostream>

using namespace ns3;

/**
 * Tutorial 4 - a simple Object to show how to hook a trace.
 */
 /* Criando um objeto que será monitorado. Alterações feitas em um de seus atributos serão registradas em log. */
class MyObject : public Object
{
  public:
    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("MyObject")
                                .SetParent<Object>()
                                .SetGroupName("Tutorial")
                                .AddConstructor<MyObject>()
								//Fornece o gancho necessário para conectar a fonte do dado ao mundo externo.
                                .AddTraceSource("MyInteger", //ID da fonte de rastreamento
                                                "An integer value to trace.", //String de ajuda
                                                MakeTraceSourceAccessor(&MyObject::m_myInt), //Define o atributo que será rastreado
                                                "ns3::TracedValueCallback::Int32"); //tipo do atributo
        return tid;
    }

    MyObject()
    {
    }

	//Valor que será rastreado. Precisa ser do tipo TracedValue.
    TracedValue<int32_t> m_myInt; //!< The traced value.
};


/** Função que será chamada para imprimir o valor rastreado em log. */
void IntTrace(int32_t oldValue, int32_t newValue)
{
    std::cout << "Traced " << oldValue << " to " << newValue << std::endl;
}

int main(int argc, char* argv[])
{
	//Cria um objeto que possui monitoramento
    Ptr<MyObject> myObject = CreateObject<MyObject>();
	
	//Faz a conexão do objeto com a função de rastreio
    myObject->TraceConnectWithoutContext("MyInteger", MakeCallback(&IntTrace));

    //Altera o valor do atributo rastreado
	myObject->m_myInt = 1234;

    return 0;
}
